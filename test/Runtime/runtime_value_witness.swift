// RUN: %target-run-simple-swift(-g -Onone -Xfrontend -enable-type-layout -Xfrontend -enable-runtime-value-witness) --stdlib-unittest-in-process

// REQUIRES: executable_test

import StdlibUnittest

var Tests = TestSuite("RuntimeValueWitness")

Tests.test("MultiReferenceStruct") {
  struct LifetimeStruct {
    init(a: LifetimeTracked, b: LifetimeTracked, c: LifetimeTracked) {
      self.a = a
      self.b = b
      self.c = c
    }
    let a: LifetimeTracked
    let b: LifetimeTracked
    let c: LifetimeTracked 
  }
  let _ = LifetimeStruct(a: LifetimeTracked(0), b: LifetimeTracked(0), c: LifetimeTracked(0))
}

Tests.test("AlignedStruct") {
  struct AlignedStruct {
    init(a: UInt8, b: UInt16, c: LifetimeTracked) {
      self.a = a
      self.b = b
      self.c = c
    }
    let a: UInt8
    let b: UInt16
    let c: LifetimeTracked
  }
  let _ = AlignedStruct(a: 0xAA, b: 0xBBBB, c: LifetimeTracked(0))
}

Tests.test("NestedStruct") {
  struct NestedStruct {
    init(a: UInt8, b: LifetimeTracked) {
      self.a = a
      self.b = b
    }
    let a: UInt8
    let b: LifetimeTracked
  }
  struct OuterStruct {
    init(a: UInt8, b: NestedStruct) {
      self.a = a
      self.b = b
    }
    let a: UInt8
    let b: NestedStruct
  }
  // We should expect to see a layout of AA00000000000000 BB00000000000000 POINTER
  // As the nested struct, and thus b, will be pointer aligned
  let _ = OuterStruct(a: 0xAA, b: NestedStruct(a: 0xBB, b: LifetimeTracked(0)))
}

Tests.test("FlattenedStruct") {
  struct FlattenedStruct {
    init(a: UInt8, b: UInt8, c: LifetimeTracked) {
      self.a = a
      self.b = b
      self.c = c
    }
    let a: UInt8
    let b: UInt8
    let c: LifetimeTracked
  }
  // We should expect to see a layout of AABB000000000000 POINTER
  // As the layout should pack a and b.
  let _ = FlattenedStruct(a: 0xAA, b: 0xBB, c: LifetimeTracked(0))
}

Tests.test("NoPayloadEnumStruct") {
  enum NoPayload {
    case Only
    case NonPayload
    case Cases
  }
  struct NoPayloadEnumStruct {
    init(a: NoPayload, c: LifetimeTracked) {
      self.a = a
      self.c = c
    }
    let a: NoPayload
    let c: LifetimeTracked
  }
  let _ = NoPayloadEnumStruct(a: .Cases, c: LifetimeTracked(0))
}

Tests.test("SinglePayloadEnumStruct") {
  enum SinglePayload {
    case Payload(c: LifetimeTracked)
    case NoPayload
  }
  struct SinglePayloadEnumStruct {
    init(a: SinglePayload, c: LifetimeTracked) {
      self.a = a
      self.c = c
    }
    let a: SinglePayload
    let c: LifetimeTracked
  }
  let _ = SinglePayloadEnumStruct(a: .Payload(c: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = SinglePayloadEnumStruct(a: .NoPayload, c: LifetimeTracked(0))
}

Tests.test("Nested Enum") {
  enum SinglePayload {
    case Payload(c: LifetimeTracked)
    case NoPayload
  }
  enum SingleEnumPayload {
    case EnumPayload(e: SinglePayload)
    case NoEnumPayload
  }
  struct SingleEnumPayloadStruct {
    init(a: SingleEnumPayload, c: LifetimeTracked) {
      self.a = a
      self.c = c
    }
    let a: SingleEnumPayload
    let c: LifetimeTracked
  }
  let _ = SingleEnumPayloadStruct(a: .EnumPayload(e: .Payload(c: LifetimeTracked(0))), c: LifetimeTracked(0))
  let _ = SingleEnumPayloadStruct(a: .EnumPayload(e: .NoPayload), c: LifetimeTracked(0))
  let _ = SingleEnumPayloadStruct(a: .NoEnumPayload, c: LifetimeTracked(0))
}

Tests.test("MultiEnum") {
  enum MultiPayload {
    case Payload1(c: LifetimeTracked)
    case Payload2(c: LifetimeTracked, d: LifetimeTracked)
    case Payload3(c: LifetimeTracked, d: LifetimeTracked)
    case Payload4(c: LifetimeTracked, d: LifetimeTracked)
    case Payload5(c: LifetimeTracked, d: LifetimeTracked)
    case Payload6(c: LifetimeTracked, d: LifetimeTracked)
    case Payload7(c: LifetimeTracked, d: LifetimeTracked)
    case Payload8(c: LifetimeTracked, d: LifetimeTracked)
    case Payload9(c: LifetimeTracked, d: LifetimeTracked)
    case Payload10(c: LifetimeTracked, d: LifetimeTracked, e: LifetimeTracked)
    case Payload11(c: LifetimeTracked, d: LifetimeTracked, e: LifetimeTracked, f: UInt64)
    case NoPayload
    case NoPayload2
    case NoPayload3
    case NoPayload4
  }

  struct MultiPayloadStruct {
    init(a: MultiPayload, c: LifetimeTracked) {
      self.a = a
      self.c = c
    }
    let a: MultiPayload
    let c: LifetimeTracked
  }
  let _ = MultiPayloadStruct(a: .Payload1(c: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload2(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload3(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload4(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload5(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload6(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload7(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload8(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload9(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload10(c: LifetimeTracked(0), d: LifetimeTracked(0), e: LifetimeTracked(0)), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .Payload11(c: LifetimeTracked(0), d: LifetimeTracked(0), e: LifetimeTracked(0), f: 0xAAAAAAAAAAAAAAAA), c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .NoPayload, c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .NoPayload2, c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .NoPayload3, c: LifetimeTracked(0))
  let _ = MultiPayloadStruct(a: .NoPayload4, c: LifetimeTracked(0))
}

Tests.test("Archetypes") {
  struct ArchetypeStruct<T> {
    init(a: T, b: LifetimeTracked) {
      self.a = a
      self.b = b
    }
    let a: T
    let b: LifetimeTracked
  }

  let _ = ArchetypeStruct<UInt64>(a: 0xAAAA, b: LifetimeTracked(0))
  let _ = ArchetypeStruct<UInt32>(a: 0xBBBB, b: LifetimeTracked(0))
  let _ = ArchetypeStruct<UInt16>(a: 0xCCCC, b: LifetimeTracked(0))
  let _ = ArchetypeStruct<UInt8>(a: 0xDD, b: LifetimeTracked(0))
  let _ = ArchetypeStruct<LifetimeTracked>(a: LifetimeTracked(0), b: LifetimeTracked(0))
}

Tests.test("Multi Archetypes") {
  struct ArchetypeStruct<S, T> {
    init(a: S, b: LifetimeTracked, c: T) {
      self.a = a
      self.b = b
      self.c = c
    }
    let a: S
    let b: LifetimeTracked
    let c: T
  }

  let _ = ArchetypeStruct<LifetimeTracked, LifetimeTracked>(a: LifetimeTracked(0), b: LifetimeTracked(0), c: LifetimeTracked(0))
  let _ = ArchetypeStruct<UInt64, UInt64>(a: 0xAAAA, b: LifetimeTracked(0), c: 0)
  let _ = ArchetypeStruct<UInt64, LifetimeTracked>(a: 0xAAAA, b: LifetimeTracked(0), c: LifetimeTracked(0))
}

runAllTests()
