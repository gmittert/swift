//===--- RuntimeValueWitness.cpp - Swift Language Value Witness Runtime Implementation---===//
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

#include <cstdint>
#include <iostream>
#include <sstream>
#include "RuntimeValueWitness.h"
#include "swift/Runtime/Error.h"
#include "swift/Runtime/HeapObject.h"
#include "swift/ABI/MetadataValues.h"
#include "llvm/IR/Intrinsics.h"
#include <Block.h>

using namespace swift;


// A simple version of swift/Basic/ClusteredBitVector that doesn't use llvm::APInt
struct BitVector {
  std::vector<uint8_t> data;
  BitVector() = default;
  BitVector(std::vector<uint8_t> values) {
    data = values;
  }

  std::string asStr() {
    std::stringstream ss;
    for (uint8_t byte: data) {
      ss << std::hex << (uint32_t)byte << ",";
    }
    return ss.str();
  }

  size_t count() {
    size_t total = 0;
    for (uint8_t byte : data) {
      total += ((byte & 1) == 0) + ((byte & 2) == 0) + ((byte & 4) == 0) +
               ((byte & 8) == 0) + ((byte & 16) == 0) + ((byte & 32) == 0) +
               ((byte & 64) == 0) + ((byte & 128) == 0);
    }
    return total;
  }

  void add(std::vector<uint8_t> values) {
      data.insert(data.end(), values.begin(), values.end());
  }

  void add(BitVector v) {
      data.insert(data.end(), v.data.begin(), v.data.end());
  }

  bool none() {
    for (uint8_t byte : data) {
      if (byte != 0) {
        return false;
      }
    }
    return true;
  }

  bool any() {
    return !none();
  }

  /// return size in bits
  size_t size() {
    return data.size() * 8;
  }

  BitVector& operator&=(const BitVector& other) {
    assert(other.size() == size() && "cannot & differently sized bit vectors");
    for (uint i = 0; i < data.size(); i++) {
      data[i] &= other.data[i];
    }
    return *this;
  }
};

static BitVector spareBits(const uint8_t *typeLayout, size_t layoutLength);

/// Given a pointer and an offset, read the requested data and increment the offset
template <typename T> T readBytes(const uint8_t* typeLayout, size_t& i) {
  T returnVal = *(const T*)(typeLayout + i);
  for (uint j = 0; j < sizeof(T); j++) {
    returnVal <<= 8;
    returnVal |= *(typeLayout + i + j);
  }
  i += sizeof(T);
  return returnVal;
}

static BitVector pointerSpareBitMask() {
  // Only the bottom 56 bits are used, and heap objects are eight-byte-aligned.
  return BitVector(std::vector<uint8_t>({0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}));
}

static uint32_t countExtraInhabitants(BitVector bitMask) {
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
  if (bitMask.none()) return 0;
  // Make sure the arithmetic below doesn't overflow.
  if (bitMask.size() >= 32)
    // Max Num Extra Inhabitants: 0x7FFFFFFF (i.e. we need at least one
    // inhabited bit)
    return ValueWitnessFlags::MaxNumExtraInhabitants;
  uint32_t spareBitCount = bitMask.count();
  uint32_t nonSpareCount = bitMask.size() - bitMask.count();
  uint32_t rawCount = ((1U << spareBitCount) - 1U) << nonSpareCount;
  return std::min(rawCount,
                  unsigned(ValueWitnessFlags::MaxNumExtraInhabitants));
}

static size_t computeSize(const uint8_t *typeLayout, size_t layoutLength) {
  size_t addr = 0;
  for (size_t i = 0; i < layoutLength; i++) {
    switch ((LayoutType)typeLayout[i]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      uint64_t shiftValue = uint64_t(1) << (typeLayout[i] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = ((uint64_t)addr + alignMask) & (~alignMask);
      break;
    }
    case LayoutType::I8:
      addr += 1;
      break;
    case LayoutType::I16:
      addr += 2;
      break;
    case LayoutType::I32:
      addr += 4;
      break;
    case LayoutType::I64:
      addr += 8;
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
      break;
    case LayoutType::ThickFunc:
      addr += 16;
      break;
    case LayoutType::MultiPayloadEnum: {
      std::cout << "================================ ASSERT FAIL ==============" << std::endl;
      assert(false && "TODO");
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      // SINGLEENUM := 'e' SIZE SIZE VALUE
      // e NumEmptyPayloads LengthOfPayload Payload

      // Read 'e'
      readBytes<uint8_t>(typeLayout, i);
      uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, i);
      uint32_t payloadLength = readBytes<uint32_t>(typeLayout, i);
      size_t payloadSize = computeSize(typeLayout + i, payloadLength);

      BitVector payloadSpareBits = spareBits(typeLayout+i, payloadLength);
      uint32_t numExtraInhabitants = countExtraInhabitants(payloadSpareBits);
      uint32_t tagsInExtraInhabitants = std::min(numExtraInhabitants, numEmptyPayloads);
      uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

      uint8_t tagSize = spilledTags == 0           ? 0
                        : spilledTags < UINT8_MAX  ? 1
                        : spilledTags < UINT16_MAX ? 2
                                                   : 4;

      addr += payloadSize + tagSize;
      i += payloadLength - 1;
    }
    }
  }
  return addr;
}

static BitVector spareBits(const uint8_t *typeLayout, size_t layoutLength) {
  BitVector bitVector;

  uint64_t addr = 0;
  // In an aligned group, we use the field with the most extra inhabitants,
  // favouring the earliest field in a tie.
  for (size_t i = 0; i < layoutLength; i++) {
    switch ((LayoutType)typeLayout[i]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      // Despite indicating it does, Swift codegen doesn't appear to use alignment
      // space for spare bits yet.
      uint64_t addr_prev = addr;
      uint64_t shiftValue = uint64_t(1) << (typeLayout[i] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = ((uint64_t)addr + alignMask) & (~alignMask);
      for (uint j = 0; j < addr- addr_prev; j++) {
        bitVector.add({0x0});
      }
      break;
    }
    case LayoutType::I8:
      bitVector.add({0x00});
      addr += 1;
      break;
    case LayoutType::I16:
      bitVector.add({0x00, 0x00});
      addr += 2;
      break;
    case LayoutType::I32:
      bitVector.add({0x00, 0x00, 0x00, 0x00});
      addr += 2;
      addr += 4;
      break;
    case LayoutType::I64:
      bitVector.add({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
      addr += 8;
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
      break;
    }
    case LayoutType::ThickFunc: {
      bitVector.add({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
      bitVector.add(pointerSpareBitMask());
      addr += 16;
      break;
    }
    case LayoutType::MultiPayloadEnum: {
      std::cout << "================================ ASSERT FAIL ==============" << std::endl;
      assert(false && "TODO");
      break;
    }
    case LayoutType::SinglePayloadEnum: {
      // Read 'e'
      readBytes<uint8_t>(typeLayout, i);
      uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, i);
      uint32_t payloadLength = readBytes<uint32_t>(typeLayout, i);
      size_t payloadSize = computeSize(typeLayout + i, payloadLength);

      BitVector payloadSpareBits = spareBits(typeLayout+i, payloadLength);
      uint32_t numExtraInhabitants = countExtraInhabitants(payloadSpareBits);
      uint32_t tagsInExtraInhabitants = std::min(numExtraInhabitants, numEmptyPayloads);
      uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

      uint8_t tagSize = spilledTags == 0           ? 0
                        : spilledTags < UINT8_MAX  ? 1
                        : spilledTags < UINT16_MAX ? 2
                                                   : 4;

      for (uint i = 0; i < payloadSize + tagSize; i++) {
        bitVector.add({0x00});
      }
      addr += payloadSize + tagSize;
      i += payloadLength - 1;
      break;
    }
  }
  }
  return bitVector;
}


SWIFT_RUNTIME_EXPORT
void swift::swift_value_witness_destroy(void *address, const uint8_t *typeLayout,
                                 uint32_t layoutLength) {
  uint8_t* addr = (uint8_t*)address;
  for (size_t i = 0; i < layoutLength; i++) {
    switch ((LayoutType)typeLayout[i]) {
    case LayoutType::Align0:
    case LayoutType::Align1:
    case LayoutType::Align2:
    case LayoutType::Align4:
    case LayoutType::Align8:
    case LayoutType::Align16:
    case LayoutType::Align32:
    case LayoutType::Align64: {
      uint64_t shiftValue = uint64_t(1) << (typeLayout[i] - '0');
      uint64_t alignMask = shiftValue - 1;
      addr = (uint8_t*)(((uint64_t)addr + alignMask) & (~alignMask));
      break;
    }
    case LayoutType::I8:
      addr += 1;
      break;
    case LayoutType::I16:
      addr += 2;
      break;
    case LayoutType::I32:
      addr += 4;
      break;
    case LayoutType::I64:
      addr += 8;
      break;
    case LayoutType::ErrorReference:
      swift_errorRelease(*(SwiftError**)addr);
      addr += 8;
      break;
    case LayoutType::NativeStrongReference:
      swift_release(*(HeapObject**)addr);
      addr += 8;
      break;
    case LayoutType::NativeUnownedReference:
      swift_release(*(HeapObject**)addr);
      addr += 8;
      break;
    case LayoutType::NativeWeakReference:
      swift_weakDestroy(*(WeakReference**)addr);
      addr += 8;
      break;
    case LayoutType::UnknownUnownedReference:
      swift_unknownObjectUnownedDestroy(*(UnownedReference**) addr);
      addr += 8;
      break;
    case LayoutType::UnknownWeakReference:
      swift_unknownObjectWeakDestroy(*(WeakReference**)addr);
      addr += 8;
      break;
    case LayoutType::BlockReference:
      //_Block_release(addr);
      addr += 8;
      break;
    case LayoutType::BridgeReference:
      swift_bridgeObjectRelease(*(HeapObject**)addr);
      addr += 8;
      break;
    case LayoutType::ObjCReference:
#if defined(__APPLE__)
      objc_release(*(HeapObject**)addr);
      addr += 8;
      break;
#endif
    case LayoutType::ThickFunc:
      swift_release(*((HeapObject**)addr+1));
      addr += 16;
      break;
    case LayoutType::SinglePayloadEnum: {
      // SINGLEENUM := 'e' SIZE SIZE VALUE
      // e NumEmptyPayloads LengthOfPayload Payload

      // Read 'e'
      readBytes<uint8_t>(typeLayout, i);
      uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, i);
      uint32_t payloadLength = readBytes<uint32_t>(typeLayout, i);
      size_t payloadSize = computeSize(typeLayout + i, payloadLength);

      BitVector payloadSpareBits = spareBits(typeLayout+i, payloadLength);
      uint32_t numExtraInhabitants = countExtraInhabitants(payloadSpareBits);
      uint32_t tagsInExtraInhabitants = std::min(numExtraInhabitants, numEmptyPayloads);
      uint32_t spilledTags = numEmptyPayloads - tagsInExtraInhabitants;

      uint8_t tagSize = spilledTags == 0           ? 0
                        : spilledTags < UINT8_MAX  ? 1
                        : spilledTags < UINT16_MAX ? 2
                                                   : 4;
      if (tagSize > 0) {
        uint64_t tag = tagSize == 1   ? *(uint8_t *)(addr + payloadSize)
                       : tagSize == 2 ? *(uint16_t *)(addr + payloadSize)
                                      : *(uint32_t *)(addr + payloadSize);
        if (tag != 0) {
          addr += payloadSize + tagSize;
          i += payloadLength - 1;
          break;
        }
      }
      
      if (tagsInExtraInhabitants >= 0) {
        BitVector masked;
        for (uint i = 0; i < payloadSize; i++) {
          masked.add({addr[i]});
        }
        masked &= payloadSpareBits;

        if (masked.any()) {
          addr += payloadSize + tagSize;
          i += payloadLength - 1;
          break;
        }
      }

      swift::swift_value_witness_destroy((void *)addr, typeLayout + i,
                                         payloadLength);
      addr += payloadSize + tagSize;
      i += payloadLength - 1;
      break;
    }
    case LayoutType::MultiPayloadEnum:
      // MULTIENUM := 'E' SIZE SIZE SIZE+ VALUE+
      // E numEmptyPayloads numPayloads legnthOfEachPayload payloads

      // Read 'E'
      readBytes<uint8_t>(typeLayout, i);
      uint32_t numEmptyPayloads = readBytes<uint32_t>(typeLayout, i);
      uint32_t numPayloads = readBytes<uint32_t>(typeLayout, i);
      size_t payloadSize = computeSize(typeLayout+i, layoutLength);

      // numExtraInhabitants
      break;

  }
  }
}
