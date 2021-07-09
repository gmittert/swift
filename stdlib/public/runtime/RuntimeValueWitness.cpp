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
#include "swift/ABI/Metadata.h"
#include "swift/Runtime/Error.h"
#include "swift/Runtime/HeapObject.h"
#include <Block.h>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <sys/types.h>

using namespace swift;

static uint8_t bitsToRepresent(uint32_t numValues) {
  unsigned int r = 1;
  while (numValues >>= 1) {
    r++;
  }
  return r;
}

static uint8_t bytesToRepresent(uint32_t numValues) {
  return (bitsToRepresent(numValues) + 7) / 8;
}

// A simple version of swift/Basic/ClusteredBitVector that doesn't use
// llvm::APInt
BitVector::BitVector(std::vector<uint8_t> values) { add(values); }
BitVector::BitVector(size_t bits) { data = std::vector<bool>(bits, 0); }

std::string BitVector::asStr() const {
  std::stringstream ss;
  ss << std::hex;
  int byteCounter = 0;
  for (uint i = 0; i < data.size();) {
    uint8_t byte = 0;
    for (int j = 0; j < 8; j++) {
      byte <<= 1;
      byte |= data[i++];
    }
    byteCounter++;
    if (byte < 0x10) {
      ss << (uint32_t)0;
    }
    ss << (uint32_t)byte;
    if (byteCounter % 2 == 0) {
      ss << " ";
    }
    if (byteCounter % 4 == 0) {
      ss << " ";
    }
    if (byteCounter % 8 == 0) {
      ss << " ";
    }
  }
  return ss.str();
}

size_t BitVector::count() const {
  size_t total = 0;
  for (auto bit : data) {
    total += bit;
  }
  return total;
}

void BitVector::add(uint8_t byte) {
  for (uint i = 0; i < 8; i++) {
    data.push_back(byte >> (7 - i) & 0x1);
  }
}

void BitVector::add(std::vector<uint8_t> values) {
  for (auto value : values) {
    add(value);
  }
}

void BitVector::add(BitVector v) {
  data.insert(data.end(), v.data.begin(), v.data.end());
}

bool BitVector::none() const {
  for (bool bit : data) {
    if (bit != 0) {
      return false;
    }
  }
  return true;
}

void BitVector::zextTo(size_t numBits) {
  if (numBits <= size())
    return;
  auto zeros = std::vector<bool>((numBits - size()), 0);
  data.insert(data.begin(), zeros.begin(), zeros.end());
}

void BitVector::truncateTo(size_t numBits) {
  auto other = std::vector<bool>(data.begin(), data.begin() + numBits);
  data = other;
}

void BitVector::zextOrTruncTo(size_t numBits) {
  if (numBits > size()) {
    truncateTo(numBits);
  } else {
    zextTo(numBits);
  }
}

bool BitVector::any() const { return !none(); }

size_t BitVector::size() const { return data.size(); }

BitVector &BitVector::operator&=(const BitVector &other) {
  for (uint i = 0; i < size(); i++) {
    data[i] = data[i] & other.data[i];
  }
  return *this;
}

BitVector BitVector::operator+(const BitVector &other) const {
  BitVector bv = *this;
  bv.data.insert(bv.data.end(), other.data.begin(), other.data.end());
  return bv;
}

uint32_t BitVector::toI32() const {
  uint32_t output = 0;
  for (auto bit : data) {
    output <<= 1;
    output |= bit;
  }
  return output;
}

void BitVector::maskBits(uint8_t *addr) {
  uint8_t *ptr = addr;
  std::vector<uint8_t> wordVec;
  for (uint i = 0; i < data.size();) {
    uint8_t byte = 0;
    for (int j = 0; j < 8; j++) {
      byte <<= 1;
      byte |= data[i++];
    }
    wordVec.push_back(byte);
    if (wordVec.size() == 8) {
      for (auto byte = wordVec.rbegin(); byte != wordVec.rend(); byte++) {
        *ptr = *ptr & *byte;
        ptr++;
      }
      wordVec.clear();
    }
  }
}

uint32_t BitVector::gatherBits(BitVector mask) {
  auto masked = *this;
  masked &= mask;
  assert(mask.size() == size());

  uint32_t output = 0;
  for (uint i = 0; i < mask.size(); i++) {
    auto maskBit = mask.data[i];
    auto valueBit = data[i];
    if (maskBit) {
      output <<= 1;
      output |= valueBit;
    }
  }
  return output;
}

uint32_t indexFromValue(BitVector mask, BitVector payload,
                        BitVector extraTagBits) {
  unsigned numSpareBits = mask.count();
  uint32_t tag = 0;

  // Get the tag bits from spare bits, if any.
  if (numSpareBits > 0) {
    tag = payload.gatherBits(mask);
  }

  // Merge the extra tag bits, if any.
  if (extraTagBits.size() > 0) {
    tag = (extraTagBits.toI32() << numSpareBits) | tag;
  }
  return tag;
}

BitVector BitVector::getBitsSetFrom(uint32_t numBits, uint32_t lobits) {
  BitVector bv;
  uint8_t byte = 0;
  while (numBits > 0) {
    byte <<= 1;
    if (numBits > lobits) {
      byte |= 1;
    }
    numBits--;
    if (numBits % 8 == 0) {
      bv.add(byte);
      byte = 0;
    }
  }
  return bv;
}

BitVector BitVector::operator~() const {
  BitVector result = *this;
  result.data.flip();
  return result;
}

uint32_t BitVector::countExtraInhabitants() const {
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
uint8_t readBytes(const uint8_t *typeLayout, size_t &i) {
  uint8_t returnVal = *(const uint8_t *)(typeLayout + i);
  i += 1;
  return returnVal;
}

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
AlignedGroup readAlignedGroup(const uint8_t *typeLayout, size_t &offset, Metadata* metadata) {
  // a numFields (alignment,fieldLength,field)+
  // ALIGNED_GROUP:= 'a' SIZE (ALIGNMENT SIZE VALUE)+
  readBytes<uint8_t>(typeLayout, offset);
  uint32_t numFields = readBytes<uint32_t>(typeLayout, offset);
  std::vector<AlignedGroup::Field> fields;
  for (uint i = 0; i < numFields; i++) {
    uint8_t alignment = readBytes<uint8_t>(typeLayout, offset);
    uint32_t layoutLen = readBytes<uint32_t>(typeLayout, offset);
    fields.emplace_back(alignment, layoutLen, typeLayout+offset);
    offset += layoutLen;
  }
  return AlignedGroup(fields, metadata);
}

SinglePayloadEnum readSinglePayloadEnum(const uint8_t *typeLayout,
                                        size_t &offset, Metadata* metadata) {
  // SINGLEENUM := 'e' SIZE SIZE VALUE
  // e NumEmptyPayloads LengthOfPayload Payload

  // Read 'e'
  readBytes<uint8_t>(typeLayout, offset);
  uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, offset);
  uint32_t payloadLayoutLength = readBytes<uint32_t>(typeLayout, offset);
  const uint8_t *payloadLayoutPtr = typeLayout + offset;
  uint32_t payloadSize = computeSize(payloadLayoutPtr, payloadLayoutLength, metadata);

  BitVector payloadSpareBits = spareBits(payloadLayoutPtr, payloadLayoutLength, metadata);
  uint32_t numExtraInhabitants = payloadSpareBits.countExtraInhabitants();

  uint32_t tagsInExtraInhabitants =
      std::min(numExtraInhabitants, numEmptyPayloads);
  uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

  uint8_t tagSize = spilledTags == 0           ? 0
                    : spilledTags < UINT8_MAX  ? 1
                    : spilledTags < UINT16_MAX ? 2
                                               : 4;
  BitVector enumSpareBits = payloadSpareBits;
  for (uint i = 0; i < tagSize; i++) {
    enumSpareBits.add(0x00);
  }
  offset += payloadLayoutLength;
  return SinglePayloadEnum(numEmptyPayloads, payloadLayoutLength, payloadSize,
                           tagsInExtraInhabitants, enumSpareBits,
                           payloadSpareBits, payloadLayoutPtr, tagSize, metadata);
}

uint32_t MultiPayloadEnum::payloadSize() const {
  size_t maxSize = 0;
  for (uint i = 0; i < payloadLayoutPtr.size(); i++) {
    maxSize = std::max(maxSize, computeSize(payloadLayoutPtr[i], payloadLayoutLength[i], metadata));
  }
  return maxSize;
}
MultiPayloadEnum readMultiPayloadEnum(const uint8_t *typeLayout,
                                      size_t &offset, Metadata* metadata) {
  // // E numEmptyPayloads numPayloads lengthOfEachPayload payloads
  // MULTIENUM := 'E' SIZE SIZE SIZE+ VALUE+

  // Read 'E'
  readBytes<uint8_t>(typeLayout, offset);
  uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, offset);
  uint32_t numPayloads = readBytes<uint32_t>(typeLayout, offset);
  std::vector<uint32_t> payloadLengths;
  std::vector<const uint8_t *> payloadOffsets;
  std::vector<BitVector> casesSpareBits;
  for (uint i = 0; i < numPayloads; i++) {
    payloadLengths.push_back(readBytes<uint32_t>(typeLayout, offset));
  }
  for (auto payloadLength : payloadLengths) {
    payloadOffsets.push_back(typeLayout + offset);
    casesSpareBits.push_back(spareBits(typeLayout + offset, payloadLength, metadata));
    offset += payloadLength;
  }

  BitVector payloadSpareBits;
  for (auto vec : casesSpareBits) {
    if (vec.size() > payloadSpareBits.size()) {
      payloadSpareBits.data.insert(
          payloadSpareBits.data.end(),
          vec.data.begin() + payloadSpareBits.data.size(), vec.data.end());
    }
    payloadSpareBits &= vec;
  }

  uint32_t numExtraInhabitants = payloadSpareBits.countExtraInhabitants();
  uint32_t tagsInExtraInhabitants =
      std::min(numExtraInhabitants, numEmptyPayloads);
  uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

  // round up to the nearest byte
  uint8_t extraTagBytes = bytesToRepresent(spilledTags);

  BitVector extraTagBitsSpareBits(extraTagBytes);
  // If we rounded up, those extra tag bits are spare.
  for (int i = 0; i < extraTagBytes * 8 - bitsToRepresent(spilledTags); i++) {
    extraTagBitsSpareBits.data[i] = 1;
  }

  return MultiPayloadEnum(numEmptyPayloads, payloadLengths,
                          extraTagBitsSpareBits, payloadOffsets, metadata);
}

const BitVector MultiPayloadEnum::commonSpareBits() const {
  BitVector bits = ~BitVector(payloadSize()* 8);
  for (uint i = 0; i < payloadLayoutPtr.size(); i++) {
    auto caseSpareBits = ::spareBits(payloadLayoutPtr[i], payloadLayoutLength[i], metadata);
    auto numTrailingZeros = payloadSize()*8 - caseSpareBits.size();
    auto extended = caseSpareBits + ~BitVector(numTrailingZeros);
    bits &= extended;
  }
  return bits;
}

uint32_t MultiPayloadEnum::tagsInExtraInhabitants() const {
  return std::min(commonSpareBits().countExtraInhabitants(), numEmptyPayloads);
}

uint32_t MultiPayloadEnum::size() const { return payloadSize() + extraTagBitsSpareBits.size()/8; }

BitVector MultiPayloadEnum::spareBits() const {
  return commonSpareBits() + extraTagBitsSpareBits;
}

static BitVector pointerSpareBitMask() {
  // Only the bottom 56 bits are used, and heap objects are eight-byte-aligned.
  return BitVector(
      std::vector<uint8_t>({0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07}));
}

size_t computeSize(const uint8_t *typeLayout, size_t layoutLength, Metadata* metadata) {
  size_t addr = 0;
  size_t offset = 0;
  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::AlignedGroup: {
      addr += readAlignedGroup(typeLayout, offset, metadata).size();
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
      addr += readMultiPayloadEnum(typeLayout, offset, metadata).size();
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      addr += readSinglePayloadEnum(typeLayout, offset, metadata).size;
      break;
    }
    case LayoutType::ArcheType: {
      readBytes<uint8_t>(typeLayout, offset);
      uint32_t index = readBytes<uint32_t>(typeLayout, offset);
      addr += metadata->getGenericArgs()[index]->getValueWitnesses()->getSize();
      break;
    }
    }
  }
  return addr;
}
  
BitVector spareBits(const uint8_t *typeLayout, size_t layoutLength, Metadata* metadata) {
  BitVector bitVector;

  uint64_t addr = 0;
  size_t offset = 0;
  // In an aligned group, we use the field with the most extra inhabitants,
  // favouring the earliest field in a tie.
  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::AlignedGroup: {
      AlignedGroup a = readAlignedGroup(typeLayout, offset, metadata);
      bitVector = a.spareBits();
      addr += a.size();
      break;
    }
    case LayoutType::I8:
      bitVector.add(0x00);
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
      MultiPayloadEnum e = readMultiPayloadEnum(typeLayout, offset, metadata);
      bitVector.add(e.spareBits());
      addr += e.size();
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      SinglePayloadEnum e = readSinglePayloadEnum(typeLayout, offset, metadata);
      bitVector.add(e.spareBits);
      addr += e.size;
      break;
    }
    case LayoutType::ArcheType: {
      //assert(false && "NYI");
      break;
    }
    }
  }
  return bitVector;
}


size_t AlignedGroup::size() const {
  uint64_t addr = 0;
  for (auto field : fields) {
    if (field.alignment == '?') {
      // We'd have to hit the vw table for the field's alignment, so let's just
      // get the size of the whole thing.
      return metadata->vw_size();
    }
    uint64_t shiftValue = uint64_t(1) << (field.alignment - '0');
    uint64_t alignMask = shiftValue - 1;
    addr = ((addr + alignMask) & (~alignMask));
    addr += computeSize(field.fieldPtr, field.fieldLength, metadata);
  }
  return addr;
}

BitVector AlignedGroup::spareBits() const {
  // The spare bits of an aligned group is the field that has the most number of
  // spare bits's spare bits vector, padded with zeros to the size of the struct
  //
  // In the case of a tie, we take the first field encountered.
  size_t offset = 0;
  size_t offsetOfBest = 0;
  BitVector maskOfBest;

  for (auto field : fields) {
    BitVector fieldSpareBits = ::spareBits(field.fieldPtr, field.fieldLength, metadata);
    // We're supposed to pick the first field (see
    // RecordTypeInfoImpl::getFixedExtraInhabitantProvidingField), but what
    // "first" seems to be gets reversed somewhere? So we pick the last field
    // instead, thus ">=".
    if (fieldSpareBits.count() >= maskOfBest.count()) {
      offsetOfBest = offset;
      maskOfBest = fieldSpareBits;
    }
    uint64_t shiftValue = uint64_t(1) << (field.alignment - '0');
    uint64_t alignMask = shiftValue - 1;
    offset = ((uint64_t)offset + alignMask) & (~alignMask);
    offset += computeSize(field.fieldPtr, field.fieldLength, metadata);
  }

  return BitVector(offsetOfBest*8) + maskOfBest + BitVector((size() - offsetOfBest) * 8 - maskOfBest.size());
}

uint32_t MultiPayloadEnum::gatherSpareBits(const uint8_t *data,
                                           unsigned resultBitWidth) const {
  // Use the enum's spare bit vector to mask out a value, and accumlate the
  // resulting masked value into an int of the given size.
  //
  // For example:
  // Enum Mask: 0xFF0FFF0F
  // DATA:      0x12345678
  // Produces 124568
  //
  // Enum Mask: 0xFF 0F FF 07
  // DATA:      0x12 34 56 78
  // Produces 0001 0010 0010 0100 0101 0110 111
  // ->       000 1001 0001 0010 0010 1011 0111
  // ->       1001 0001 0010 0010 1011 0111
  // ->  0xB122D9
  //
  // We are given a result bitwidth, as if we gather the required bits without
  // using all of the mask bits, we will stop.
  //
  // For example, to store the values 0-255 in a mask of 0xFFFF0000, the layout
  // algorithm only uses the minimum number of bits to represent the values.
  // 0x01000000
  // 0x02000000
  // ..
  // 0xFF000000
  // Even though we have additional bytes in the mask.
  if (!resultBitWidth) {
    return 0;
  }

  uint32_t result = 0;
  uint32_t width = 0;

  const uint8_t* addr = data;
  size_t currByteIdx = 0;
  size_t currWordIdx = 0;
  uint32_t currBitIdx = 0;

  for (auto bit: spareBits().data) {
    uint64_t currWord = *(((const uint64_t*)addr)+currWordIdx);
    uint8_t currByte = currWord >> (8*(7 - currByteIdx));
    if (bit) {
        result <<= 1;
        if ((bit << (7-currBitIdx)) & currByte) {
          result |= 1;
        }
        width += 1;
        if (width == resultBitWidth) {
          return result;
        }
    }
    currBitIdx += 1;
    if (currBitIdx == 8) {
      currBitIdx = 0;
      currByteIdx += 1;
    }

    if (currByteIdx == 8) {
      currByteIdx = 0;
      currWordIdx += 1;
    }
  }

  return result;
}

uint32_t extractPayloadTag(const MultiPayloadEnum e, const uint8_t *data) {
  unsigned numPayloads = e.payloadLayoutPtr.size();
  unsigned casesPerTag = (~e.spareBits()).count() >= 32
                             ? UINT_MAX
                             : 1U << (~e.spareBits().count());
  if (e.numEmptyPayloads != 0) {
    ;
    numPayloads += (e.numEmptyPayloads/casesPerTag) + 1;
  }

  unsigned requiredSpareBits = bitsToRepresent(numPayloads);
  unsigned numSpareBits = e.spareBits().count();
  uint32_t tag = 0;

  // Get the tag bits from spare bits, if any.
  if (numSpareBits > 0) {
    tag = e.gatherSpareBits(data, requiredSpareBits);
  }

/*
  // Get the extra tag bits, if any.
  if (e.extraTagBitsSpareBits.size() > 0) {
    if (!tag) {
      return e.extraTagBitsSpareBits.toI32();
    } else {
      uint32_t extraTagValue = 0;
      if (e.extraTagBitsSpareBits.size() == 8) {
        extraTagValue = *(const uint8_t *)data;
      } else if (e.extraTagBitsSpareBits.size() == 16) {
        extraTagValue = *(const uint16_t *)data;
      } else if (e.extraTagBitsSpareBits.size() == 32) {
        extraTagValue = *(const uint32_t *)data;
      }

      extraTagValue <<= numSpareBits;
      tag |= extraTagValue;
    }
  }
  */
  return tag;
}

SWIFT_RUNTIME_EXPORT
void swift::swift_value_witness_destroy(void *address,
                                        const uint8_t *typeLayout,
                                        uint32_t layoutLength, void *metadata) {
  Metadata* typedMetadata = (Metadata*)metadata;

  uint8_t *addr = (uint8_t *)address;
  size_t offset = 0;

  while (offset < layoutLength) {
    switch ((LayoutType)typeLayout[offset]) {
    case LayoutType::AlignedGroup: {
      AlignedGroup group = readAlignedGroup(typeLayout, offset, typedMetadata);
      for (auto field : group.fields) {
        uint64_t alignMask;
        if (field.alignment == '?') {
          alignMask = typedMetadata->getGenericArgs()[0]->vw_alignment();
        } else {
          uint64_t shiftValue = uint64_t(1) << (field.alignment - '0');
          alignMask = shiftValue - 1;
        }
        addr = (uint8_t *)(((uint64_t)addr + alignMask) & (~alignMask));
        swift_value_witness_destroy(addr, field.fieldPtr, field.fieldLength,
                                    metadata);
        addr += computeSize(field.fieldPtr, field.fieldLength, typedMetadata);
      }
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
      swift_errorRelease(*(SwiftError **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeStrongReference:
      swift_release(*(HeapObject **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeUnownedReference:
      swift_release(*(HeapObject **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::NativeWeakReference:
      swift_weakDestroy(*(WeakReference **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::UnknownUnownedReference:
      swift_unknownObjectUnownedDestroy(*(UnownedReference **)addr);
      addr += 8;
      offset++;
      break;
    case LayoutType::UnknownWeakReference:
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
      swift_release(*((HeapObject **)addr + 1));
      addr += 16;
      offset++;
      break;
    case LayoutType::SinglePayloadEnum: {
      SinglePayloadEnum e = readSinglePayloadEnum(typeLayout, offset, typedMetadata);

      if (e.tagsInExtraInhabitants > 0) {
        BitVector masked;
        std::vector<uint8_t> wordVec;
        for (uint i = 0; i < e.payloadSize; i++) {
          wordVec.push_back(addr[i]);
          if (wordVec.size() == 8) {
            // The data we read will be in little endian, so we need to reverse it byte wise.
            for (auto i = wordVec.rbegin(); i != wordVec.rend(); i++) {
              masked.add(*i);
            }
            wordVec.clear();
          }
        }

        // If we don't have a multiple of 8 bytes, we need to top off
        if (!wordVec.empty()) {
          while (wordVec.size() < 8) {
            wordVec.push_back(0);
          }
          for (auto i = wordVec.rbegin(); i != wordVec.rend(); i++) {
            masked.add(*i);
          }
        }

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

      swift::swift_value_witness_destroy((void *)addr, e.payloadLayoutPtr,
                                         e.payloadLayoutLength, metadata);
      addr += e.size;
      break;
    }
    case LayoutType::MultiPayloadEnum: {
      MultiPayloadEnum e =
          readMultiPayloadEnum(typeLayout, offset, typedMetadata);
      // numExtraInhabitants

      BitVector payloadBits;
      std::vector<uint8_t> wordVec;
      for (uint i = 0; i < e.payloadSize(); i++) {
        wordVec.push_back(addr[i]);
        if (wordVec.size() == 8) {
          // The data we read will be in little endian, so we need to reverse it
          // byte wise.
          for (auto j = wordVec.rbegin(); j != wordVec.rend(); j++) {
            payloadBits.add(*j);
          }
          wordVec.clear();
        }
      }

      // If we don't have a multiple of 8 bytes, we need to top off
      if (!wordVec.empty()) {
        while (wordVec.size() < 8) {
          wordVec.push_back(0);
        }
        for (auto i = wordVec.rbegin(); i != wordVec.rend(); i++) {
          payloadBits.add(*i);
        }
      }

      BitVector tagBits;
      for (uint i = 0; i < (e.extraTagBitsSpareBits.count() / 8); i++) {
        tagBits.add(addr[i]);
      }

      uint32_t index = extractPayloadTag(e, addr);

      // Enum indices count payload cases first, then non payload cases. Thus a
      // payload index will always be in 0...numPayloadCases-1
      if (index < e.payloadLayoutPtr.size()) {
        (~e.spareBits()).maskBits(addr);
        swift::swift_value_witness_destroy((void *)addr,
                                           e.payloadLayoutPtr[index],
                                           e.payloadLayoutLength[index], metadata);
      }
      addr += e.size();
      break;
    }
    case LayoutType::ArcheType: {
      // Read 'A'
      // Kind is a pointer sized int at offset 0 of metadata pointer
      readBytes<uint8_t>(typeLayout, offset);
      uint32_t index = readBytes<uint32_t>(typeLayout, offset);
      Metadata* typedMetadata = (Metadata*)metadata;
      typedMetadata->getGenericArgs()[index]->getValueWitnesses()->destroy((OpaqueValue*)addr, typedMetadata);
      addr += typedMetadata->getGenericArgs()[index]->getValueWitnesses()->getSize();
      break;
    }
    }
  }
}
