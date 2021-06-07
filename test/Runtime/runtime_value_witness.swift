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
//let _ = MultiPayloadStruct(a: .Payload2(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload3(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload4(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload5(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload6(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload7(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload8(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .Payload9(c: LifetimeTracked(0), d: LifetimeTracked(0)), c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .NoPayload, c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .NoPayload2, c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .NoPayload3, c: LifetimeTracked(0))
//let _ = MultiPayloadStruct(a: .NoPayload4, c: LifetimeTracked(0))
}

runAllTests()
