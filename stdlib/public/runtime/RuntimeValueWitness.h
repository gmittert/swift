//===--- RuntimeValueWitness.h- Swift Language Value Witness Runtime Implementation---===//
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

#ifndef SWIFT_RUNTIME_VALUE_WITNESS_H
#define SWIFT_RUNTIME_VALUE_WITNESS_H

#include "swift/Runtime/Config.h"
#include <cstdint>
#include <vector>

#include "../../../stdlib/public/SwiftShims/HeapObject.h"

// Layouts
//
// enum class LayoutType: char {
//   // Scalars
//   I8 = 'c',
//   I16 = 's',
//   I32 = 'l',
//   I64 = 'L',
//   ErrorReference = 'r',
//   NativeStrongReference = 'N',
//   NativeUnownedReference = 'n',
//   NativeWeakReference = 'W',
//   UnknownUnownedReference = 'u',
//   UnknownWeakReference = 'w',
//   BlockReference = 'b',
//   BridgeReference = 'B',
//   ObjCReference = 'o',
//   ThickFunc = 'f',
//   ExistentialReference = 'x',
//
//   // Enums
//   SinglePayloadEnum = 'e',
//   MultiPayloadEnum = 'E',
// };
//
// VALUE := STRUCT | ENUM | SCALAR
// SCALAR := 'c'|'s'|'l'|'L'|'C'|'r'|'N'|'n'|'W'|'u'|'w'|'b'|'B'|'o'|'f'|'x'
// ALIGNED_GROUP:= ALIGNMENT VALUE+
// ALIGNMENT := '1'|'2'|'3'|'4'|'5'|'6'|'7'
// ENUM := SINGLEENUM | MULTIENUM
//
// SIZE := uint32 (does network order this need to be specified here?)
//
// // e numEmptyPayloads lengthOfPayload payload
// SINGLEENUM := 'e' SIZE SIZE VALUE
//
// // E numEmptyPayloads numPayloads lengthOfEachPayload payloads
// MULTIENUM := 'E' SIZE SIZE SIZE+ VALUE+
//
// OFFSETS := int32+
//
// Examples:
// struct SomeStruct {
//  let a : int8
//  let b : int16
//  let c : int16
//  let d : SomeClass
// }
//
// '1c2s2s3N'
// byte aligned int8
// 2 byte aligned int16
// 2 byte aligned int16
// 4 byte aligned Native Pointer
//
// enum SampleEnum {
//    case Payload(s: SomeStruct)
//    case None
//    case ReallyNone
// }
// 'e(0x00000002)(0x00000008)1c2s2s3N'
// An enum with:
// - 2 empty cases
// - a payload stringlength of 8
// - a struct payload as in the previous example

namespace swift {
enum class LayoutType : char {
  // Scalars
  I8 = 'c',
  I16 = 's',
  I32 = 'l',
  I64 = 'L',
  ErrorReference = 'r',
  NativeStrongReference = 'N',
  NativeUnownedReference = 'n',
  NativeWeakReference = 'W',
  UnknownUnownedReference = 'u',
  UnknownWeakReference = 'w',
  BlockReference = 'b',
  BridgeReference = 'B',
  ObjCReference = 'o',
  ThickFunc = 'f',
  // Enums
  // Single
  // e{emptycases}{payload}
  //
  // Multi
  // e{emptycases}{num_cases}{payload1}{payload2}{...}{payloadn}
  SinglePayloadEnum = 'e',
  MultiPayloadEnum = 'E',
  Align0 = '0',
  Align1 = '1',
  Align2 = '2',
  Align4 = '3',
  Align8 = '4',
  Align16 = '5',
  Align32 = '6',
  Align64 = '7',
};

SWIFT_RUNTIME_EXPORT
void swift_value_witness_destroy(void *address, const uint8_t *typeLayout,
                                 uint32_t layoutLength);
} // namespace swift

struct BitVector {
  std::vector<uint8_t> data;
  BitVector() = default;
  BitVector(std::vector<uint8_t> values);

  /// Format as a String
  std::string asStr();

  /// Return the number of set bits in the bit vector
  size_t count();

  /// Append on a vector of bytes
  void add(std::vector<uint8_t> values);

  /// Append on another bitvector
  void add(BitVector v);

  /// True if all bits are 0
  bool none() const;

  /// True if any bit is a 1
  bool any() const;

  /// Number of bits in the vector
  size_t size() const;

  /// Zero extend a bit vector to a given number of bits
  void zextTo(size_t numBits);

  /// One extend a bit vector to a given number of bits
  void oextTo(size_t numBits);

  /// Bitwise &= with another vector of the same length
  BitVector &operator&=(const BitVector &other);

  /// Return a new bitvector that is the bitwise not of this one
  BitVector operator~() const;

  /// Number of extra inhabitants that fit into this vector
  uint32_t countExtraInhabitants();
};

uint32_t indexFromExtraInhabitants(BitVector mask, BitVector value);

uint32_t extractBits(BitVector mask, BitVector value);
BitVector spareBits(const uint8_t *typeLayout, size_t layoutLength);
size_t computeSize(const uint8_t *typeLayout, size_t layoutLength);

struct SinglePayloadEnum {
  SinglePayloadEnum(uint32_t numEmptyPayloads, uint32_t payloadLayoutLength,
                    uint32_t payloadSize, uint32_t tagsInExtraInhabitants,
                    BitVector spareBits, BitVector payloadSpareBits,
                    const uint8_t *payloadLayoutPtr, uint8_t tagSize)
      : numEmptyPayloads(numEmptyPayloads),
        payloadLayoutLength(payloadLayoutLength),
        payloadLayoutPtr(payloadLayoutPtr), payloadSize(payloadSize),
        tagsInExtraInhabitants(tagsInExtraInhabitants), spareBits(spareBits),
        payloadSpareBits(payloadSpareBits), tagSize(tagSize),
        size(payloadSize + tagSize) {}
  uint32_t numEmptyPayloads;
  uint32_t payloadLayoutLength;
  const uint8_t *payloadLayoutPtr;
  uint32_t payloadSize;
  uint32_t tagsInExtraInhabitants;
  BitVector spareBits;
  BitVector payloadSpareBits;
  uint8_t tagSize;
  uint8_t size;
};
struct MultiPayloadEnum {
  MultiPayloadEnum(uint32_t numEmptyPayloads,
                   std::vector<uint32_t> payloadLayoutLength,
                   uint32_t payloadSize, uint32_t tagsInExtraInhabitants,
                   BitVector spareBits, BitVector payloadSpareBits,
                   std::vector<const uint8_t *> payloadLayoutPtr,
                   uint8_t tagSize)
      : numEmptyPayloads(numEmptyPayloads),
        payloadLayoutLength(payloadLayoutLength),
        payloadLayoutPtr(payloadLayoutPtr), payloadSize(payloadSize),
        tagsInExtraInhabitants(tagsInExtraInhabitants), spareBits(spareBits),
        payloadSpareBits(payloadSpareBits), tagSize(tagSize),
        size(payloadSize + tagSize) {}
  uint32_t numEmptyPayloads;
  std::vector<uint32_t> payloadLayoutLength;
  std::vector<const uint8_t *> payloadLayoutPtr;
  uint32_t payloadSize;
  uint32_t tagsInExtraInhabitants;
  BitVector spareBits;
  BitVector payloadSpareBits;
  uint8_t tagSize;
  uint8_t size;
};
SinglePayloadEnum readSinglePayloadEnum(const uint8_t *typeLayout,
                                        size_t &offset);
MultiPayloadEnum readMultiPayloadEnum(const uint8_t *typeLayout,
                                      size_t &offset);

#endif // SWIFT_RUNTIME_VALUE_WITNESS_H
