//===--- RuntimeValueWitness.cpp - Swift Language Value Witness Runtime
//Implementation---===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// Implementations of runtime determined value witness functions
//
//===----------------------------------------------------------------------===//

#include "RuntimeValueWitness.h"
#include "swift/ABI/MetadataValues.h"
#include "swift/Runtime/Error.h"
#include "swift/Runtime/HeapObject.h"
#include <Block.h>
#include <cstdint>
#include <iostream>
#include <sstream>

using namespace swift;

// A simple version of swift/Basic/ClusteredBitVector that doesn't use
// llvm::APInt
BitVector::BitVector(std::vector<uint8_t> values) { data = values; }

std::string BitVector::asStr() {
  std::stringstream ss;
  for (uint8_t byte : data) {
    ss << std::hex << (uint32_t)byte << ",";
  }
  return ss.str();
}

size_t BitVector::count() {
  size_t total = 0;
  for (uint8_t byte : data) {
    total += ((byte & 1) == 0) + ((byte & 2) == 0) + ((byte & 4) == 0) +
             ((byte & 8) == 0) + ((byte & 16) == 0) + ((byte & 32) == 0) +
             ((byte & 64) == 0) + ((byte & 128) == 0);
  }
  return total;
}

void BitVector::add(std::vector<uint8_t> values) {
  data.insert(data.end(), values.begin(), values.end());
}

void BitVector::add(BitVector v) {
  data.insert(data.end(), v.data.begin(), v.data.end());
}

bool BitVector::none() const {
  for (uint8_t byte : data) {
    if (byte != 0) {
      return false;
    }
  }
  return true;
}

void BitVector::zextTo(size_t numBits) {
  if (numBits <= size())
    return;
  add(std::vector<uint8_t>((numBits - size()) / 8, 0x00));
}

void BitVector::oextTo(size_t numBits) {
  if (numBits <= size())
    return;
  add(std::vector<uint8_t>((numBits - size()) / 8, 0xFF));
}

bool BitVector::any() const { return !none(); }

size_t BitVector::size() const { return data.size() * 8; }

BitVector &BitVector::operator&=(const BitVector &other) {
  BitVector o = other;
  if (o.size() < size()) {
    o.oextTo(size());
  } else {
    oextTo(o.size());
  }
  for (uint i = 0; i < size() / 8; i++) {
    data[i] &= o.data[i];
  }
  return *this;
}

uint32_t extractBits(BitVector mask, BitVector value) {
  uint32_t output = 0;
  for (uint i = 0; i < mask.size() / 8; i++) {
    auto maskByte = mask.data[i];
    auto valueByte = value.data[i];
    for (uint j = 0; j < 8; j++) {
      if ((maskByte & (1 << j)) != 0) {
        output <<= 1;
        output |= (valueByte >> j) & 0x01;
      }
    }
  }
  return output;
}

uint32_t indexFromExtraInhabitants(BitVector mask, BitVector value) {
  // *Encoding the index in the extra inhabitants*
  //
  // Enums have an index from 0..n where n is the number of payload cases. To
  // encode this in the spare bits of the value, we distinguish between the
  // spare and non spare bits.
  //
  // As long as at least one spare bit is set, we do not have a payload, so we
  // can also use the non spare bits to encode our index.
  //
  // The IRGen then tries to fit as many bits of the index into the non spare
  // bits as possible, and puts the remaining into the spare bits (offsetting by
  // 1 so there is always at least one bit set).
  uint32_t setSpareBits = extractBits(mask, value);
  uint32_t setNonSpareBits = extractBits(~mask, value);
  uint32_t numNonSpareBitsUsed = std::min((~mask).count(), 31UL);
  uint32_t topBits = (setSpareBits - 1) << numNonSpareBitsUsed;
  return topBits | setNonSpareBits;
}

BitVector BitVector::operator~() const {
  BitVector result;
  for (uint8_t byte : data) {
    result.add({(uint8_t)~byte});
  }
  return result;
}
uint32_t BitVector::countExtraInhabitants() {
  // Calculating Extra Inhabitants from Spare Bits:
  // If any of the spare bits is set, we have an extra in habitant.
  //
  // Thus we have an extra inhabitant for each combination of spare bits
  // (2^#sparebits), minus 1, since all zeros indicates we have a valid value.
  //
  // Then for each combination, we can set the remaining non spare bits.
  //
  // ((2^#sparebits)-1) * (2^#nonsparebits)
  // ==> ((1 << #sparebits)-1) << #nonsparebits
  if (this->none())
    return 0;
  // Make sure the arithmetic below doesn't overflow.
  if (this->size() >= 32)
    // Max Num Extra Inhabitants: 0x7FFFFFFF (i.e. we need at least one
    // inhabited bit)
    return ValueWitnessFlags::MaxNumExtraInhabitants;
  uint32_t spareBitCount = this->count();
  uint32_t nonSpareCount = this->size() - this->count();
  uint32_t rawCount = ((1U << spareBitCount) - 1U) << nonSpareCount;
  return std::min(rawCount,
                  unsigned(ValueWitnessFlags::MaxNumExtraInhabitants));
}

/// Given a pointer and an offset, read the requested data and increment the
/// offset
template <typename T>
T readBytes(const uint8_t *typeLayout, size_t &i) {
  T returnVal = *(const T *)(typeLayout + i);
  for (uint j = 0; j < sizeof(T); j++) {
    returnVal <<= 8;
    returnVal |= *(typeLayout + i + j);
  }
  i += sizeof(T);
  return returnVal;
}

SinglePayloadEnum readSinglePayloadEnum(const uint8_t *typeLayout,
                                        size_t &offset) {
  // SINGLEENUM := 'e' SIZE SIZE VALUE
  // e NumEmptyPayloads LengthOfPayload Payload

  // Read 'e'
  std::cout << "Reading single payload enum" << std::endl;
  readBytes<uint8_t>(typeLayout, offset);
  uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, offset);
  uint32_t payloadLayoutLength = readBytes<uint32_t>(typeLayout, offset);
  std::cout << "e{numEmptyPayloads: " << numEmptyPayloads
            << "}{layoutLength: " << payloadLayoutLength << "}" << std::endl;
  const uint8_t *payloadLayoutPtr = typeLayout + offset;
  uint32_t payloadSize = computeSize(payloadLayoutPtr, payloadLayoutLength);
  std::cout << "Payload size:" << payloadSize << std::endl;

  BitVector payloadSpareBits = spareBits(payloadLayoutPtr, payloadLayoutLength);
  uint32_t numExtraInhabitants = payloadSpareBits.countExtraInhabitants();
  std::cout << "NumXI: " << std::hex << numExtraInhabitants << std::endl;

  uint32_t tagsInExtraInhabitants =
      std::min(numExtraInhabitants, numEmptyPayloads);
  uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

  uint8_t tagSize = spilledTags == 0           ? 0
                    : spilledTags < UINT8_MAX  ? 1
                    : spilledTags < UINT16_MAX ? 2
                                               : 4;
  std::cout << "Tag size: " << (uint32_t)tagSize << std::endl;
  std::cout << "Spilled tags: " << spilledTags << std::endl;
  BitVector enumSpareBits = payloadSpareBits;
  for (uint i = 0; i < tagSize; i++) {
    enumSpareBits.add({0x00});
  }
  offset += payloadLayoutLength;
  return SinglePayloadEnum(numEmptyPayloads, payloadLayoutLength, payloadSize,
                           tagsInExtraInhabitants, enumSpareBits,
                           payloadSpareBits, payloadLayoutPtr, tagSize);
}

MultiPayloadEnum readMultiPayloadEnum(const uint8_t *typeLayout,
                                      size_t &offset) {
  // // E numEmptyPayloads numPayloads lengthOfEachPayload payloads
  // MULTIENUM := 'E' SIZE SIZE SIZE+ VALUE+

  // Read 'E'
  readBytes<uint8_t>(typeLayout, offset);
  uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, offset);
  uint32_t numPayloads = readBytes<uint32_t>(typeLayout, offset);
  std::vector<uint32_t> payloadLengths;
  std::vector<const uint8_t *> payloadOffsets;
  std::vector<BitVector> casesSpareBits;
  size_t payloadSize = 0;
  for (uint i = 0; i < numPayloads; i++) {
    payloadLengths.push_back(readBytes<uint32_t>(typeLayout, offset));
  }
  for (auto payloadLength : payloadLengths) {
    payloadOffsets.push_back(typeLayout + offset);
    payloadSize =
        std::max(payloadSize, computeSize(typeLayout + offset, payloadLength));
    casesSpareBits.push_back(spareBits(typeLayout + offset, payloadLength));
    offset += payloadLength;
  }

  BitVector payloadSpareBits;
  for (auto vec : casesSpareBits) {
    payloadSpareBits &= vec;
  }

  uint32_t numExtraInhabitants = payloadSpareBits.countExtraInhabitants();
  uint32_t tagsInExtraInhabitants =
      std::min(numExtraInhabitants, numEmptyPayloads);
  uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

  uint8_t tagSize = spilledTags == 0           ? 0
                    : spilledTags < UINT8_MAX  ? 1
                    : spilledTags < UINT16_MAX ? 2
                                               : 4;

  BitVector enumSpareBits = payloadSpareBits;
  enumSpareBits.zextTo(enumSpareBits.size() + tagSize * 8);

  return MultiPayloadEnum(numEmptyPayloads, payloadLengths, payloadSize,
                          tagsInExtraInhabitants, enumSpareBits,
                          payloadSpareBits, payloadOffsets, tagSize);
}

static BitVector pointerSpareBitMask() {
  // Only the bottom 56 bits are used, and heap objects are eight-byte-aligned.
  return BitVector(
      std::vector<uint8_t>({0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}));
}

size_t computeSize(const uint8_t *typeLayout, size_t layoutLength) {
  size_t addr = 0;
  size_t offset = 0;
  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      uint64_t shiftValue = uint64_t(1) << (typeLayout[offset++] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = ((uint64_t)addr + alignMask) & (~alignMask);
      break;
    }
    case LayoutType::I8:
      addr += 1;
      offset++;
      break;
    case LayoutType::I16:
      addr += 2;
      offset++;
      break;
    case LayoutType::I32:
      addr += 4;
      offset++;
      break;
    case LayoutType::I64:
      addr += 8;
      offset++;
      break;
    case LayoutType::ErrorReference:
    case LayoutType::NativeStrongReference:
    case LayoutType::NativeUnownedReference:
    case LayoutType::NativeWeakReference:
    case LayoutType::UnknownUnownedReference:
    case LayoutType::UnknownWeakReference:
    case LayoutType::BlockReference:
    case LayoutType::BridgeReference:
    case LayoutType::ObjCReference:
      addr += 8;
      offset++;
      break;
    case LayoutType::ThickFunc:
      addr += 16;
      offset++;
      break;
    case LayoutType::MultiPayloadEnum: {
      MultiPayloadEnum e = readMultiPayloadEnum(typeLayout, offset);
      addr += e.size;
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      SinglePayloadEnum e = readSinglePayloadEnum(typeLayout, offset);
      addr += e.size;
      break;
    }
    }
  }
  return addr;
}

BitVector spareBits(const uint8_t *typeLayout, size_t layoutLength) {
  BitVector bitVector;

  uint64_t addr = 0;
  size_t offset = 0;
  // In an aligned group, we use the field with the most extra inhabitants,
  // favouring the earliest field in a tie.
  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      uint64_t addr_prev = addr;
      uint64_t shiftValue = uint64_t(1) << (typeLayout[offset++] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = ((uint64_t)addr + alignMask) & (~alignMask);
      for (uint j = 0; j < addr - addr_prev; j++) {
        bitVector.add({0x0});
      }
      break;
    }
    case LayoutType::I8:
      bitVector.add({0x00});
      addr += 1;
      offset++;
      break;
    case LayoutType::I16:
      bitVector.add({0x00, 0x00});
      addr += 2;
      offset++;
      break;
    case LayoutType::I32:
      bitVector.add({0x00, 0x00, 0x00, 0x00});
      addr += 4;
      offset++;
      break;
    case LayoutType::I64:
      bitVector.add({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
      addr += 8;
      offset++;
      break;
    case LayoutType::ErrorReference:
    case LayoutType::NativeStrongReference:
    case LayoutType::NativeUnownedReference:
    case LayoutType::NativeWeakReference:
    case LayoutType::UnknownUnownedReference:
    case LayoutType::UnknownWeakReference:
    case LayoutType::BlockReference:
    case LayoutType::BridgeReference:
    case LayoutType::ObjCReference: {
      bitVector.add(pointerSpareBitMask());
      addr += 8;
      offset++;
      break;
    }
    case LayoutType::ThickFunc: {
      bitVector.add({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
      bitVector.add(pointerSpareBitMask());
      addr += 16;
      offset++;
      break;
    }
    case LayoutType::MultiPayloadEnum: {
      MultiPayloadEnum e = readMultiPayloadEnum(typeLayout, offset);
      bitVector.add(e.spareBits);
      addr += e.size;
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      SinglePayloadEnum e = readSinglePayloadEnum(typeLayout, offset);
      bitVector.add(e.spareBits);
      addr += e.size;
      break;
    }
    }
  }
  return bitVector;
}

SWIFT_RUNTIME_EXPORT
void swift::swift_value_witness_destroy(void *address,
                                        const uint8_t *typeLayout,
                                        uint32_t layoutLength) {
  uint8_t *addr = (uint8_t *)address;
  size_t offset = 0;
  std::cout << "len: " << layoutLength << std::endl;
  std::cout << "addr: " << addr << std::endl;
  std::cout << "destroying: ";
  for (uint i = 0; i < layoutLength; i++) {
    if (typeLayout[i] < '0' || typeLayout[i] > 'z') {
      std::cout << "{";
      std::cout << (uint32_t)typeLayout[i++] << " ";
      std::cout << (uint32_t)typeLayout[i++] << " ";
      std::cout << (uint32_t)typeLayout[i++] << " ";
      std::cout << (uint32_t)typeLayout[i];
      std::cout << "}";
    } else {
      std::cout << typeLayout[i];
    }
  }
  std::cout << std::endl;
  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      uint64_t shiftValue = uint64_t(1) << (typeLayout[offset] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = (uint8_t *)(((uint64_t)addr + alignMask) & (~alignMask));
      offset++;
      break;
    }
    case LayoutType::I8:
      addr += 1;
      offset++;
      break;
    case LayoutType::I16:
      addr += 2;
      offset++;
      break;
    case LayoutType::I32:
      addr += 4;
      offset++;
      break;
    case LayoutType::I64:
      addr += 8;
      offset++;
      break;
    case LayoutType::ErrorReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_errorRelease(*(SwiftError **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeStrongReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_release(*(HeapObject **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeUnownedReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_release(*(HeapObject **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeWeakReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_weakDestroy(*(WeakReference **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::UnknownUnownedReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_unknownObjectUnownedDestroy(*(UnownedReference **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::UnknownWeakReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_unknownObjectWeakDestroy(*(WeakReference **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::BlockReference:
      //_Block_release(addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::BridgeReference:
      std::cout << "release addr: " << addr << std::endl;
      swift_bridgeObjectRelease(*(HeapObject **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::ObjCReference:
#if 0
      objc_release(*(HeapObject**)addr);
      addr += 8;
      offset++;
      break;
#endif
    case LayoutType::ThickFunc:
      std::cout << "release addr: " << addr << std::endl;
      swift_release(*((HeapObject **)addr + 1));
      addr += 16;
      offset++;
      break;
    case LayoutType::SinglePayloadEnum: {
      std::cout << "Deleting enum" << std::endl;
      SinglePayloadEnum e = readSinglePayloadEnum(typeLayout, offset);

      if (e.tagsInExtraInhabitants > 0) {
        std::cout << "Checking xi" << std::endl;
        BitVector masked;
        for (uint i = 0; i < (e.payloadSize); i++) {
          masked.add({addr[i]});
        }
        masked &= e.payloadSpareBits;

        if (masked.any()) {
          std::cout << "XI set, no destroy" << std::endl;
          addr += e.size;
          break;
        }
      }

      if (e.tagSize > 0) {
        uint8_t *tagPtr = addr + e.payloadSize;
        if (e.tagSize == 1) {
          if (*(uint8_t *)tagPtr != 0) {
            std::cout << "tag set, no destroy" << std::endl;
            addr += e.size;
            break;
          }
        } else if (e.tagSize == 2) {
          if (*(uint16_t *)tagPtr != 0) {
            std::cout << "tag set, no destroy" << std::endl;
            addr += e.size;
            break;
          }
        } else {
          if (*(uint32_t *)tagPtr != 0) {
            std::cout << "tag set, no destroy" << std::endl;
            addr += e.size;
            break;
          }
        }
      }

      std::cout << "die die die" << std::endl;
      swift::swift_value_witness_destroy((void *)addr, e.payloadLayoutPtr,
                                         e.payloadLayoutLength);
      addr += e.size;
      break;
    }
    case LayoutType::MultiPayloadEnum: {
      MultiPayloadEnum e = readMultiPayloadEnum(typeLayout, offset);
      // numExtraInhabitants

      BitVector payloadBits;
      for (uint i = 0; i < (e.payloadSize); i++) {
        payloadBits.add({addr[i]});
      }
      if (e.tagsInExtraInhabitants >= 0) {
        BitVector masked = payloadBits;
        masked &= e.payloadSpareBits;

        if (masked.any()) {
          addr += e.size;
          break;
        }
      }

      if (e.tagSize > 0) {
        uint8_t *tagPtr = addr + e.payloadSize;
        if (e.tagSize == 1) {
          if (*(uint8_t *)tagPtr != 0) {
            addr += e.size;
            break;
          }
        } else if (e.tagSize == 2) {
          if (*(uint16_t *)tagPtr != 0) {
            addr += e.size;
            break;
          }
        } else {
          if (*(uint32_t *)tagPtr != 0) {
            addr += e.size;
            break;
          }
        }
      }

      size_t index = indexFromExtraInhabitants(e.payloadSpareBits, payloadBits);
      swift::swift_value_witness_destroy((void *)addr,
                                         e.payloadLayoutPtr[index],
                                         e.payloadLayoutLength[index]);
      addr += e.size;
      break;
    }
    }
  }
  std::cout << "/destroying: " << std::endl;
}
