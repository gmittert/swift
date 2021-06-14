// RUN: %target-run-simple-swift(-Xfrontend -enable-type-layout) --stdlib-unittest-in-process

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
  struct SingleEnumPayloadEnumStruct {
    init(a: SingleEnumPayload, c: LifetimeTracked) {
      self.a = a
      self.c = c
    }
    let a: SingleEnumPayload
    let c: LifetimeTracked
  }
  let _ = SinglePayloadEnumStruct(a: .EnumPayload(.Payload(c: LifetimeTracked(0)), c: LifetimeTracked(0)))
  let _ = SinglePayloadEnumStruct(a: .EnumPayload(.NoPayload, c: LifetimeTracked(0)))
  let _ = SinglePayloadEnumStruct(a: .NoEnumPayload, c: LifetimeTracked(0))
}

runAllTests()
