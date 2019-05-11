//===--- XPCService.cpp ---------------------------------------------------===//
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

#include "sourcekitd/Logging.h"

#include "SourceKit/Core/LLVM.h"
#include "SourceKit/Support/Concurrency.h"
#include "SourceKit/Support/UIdent.h"
#include "SourceKit/Support/Logging.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Errno.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Threading.h"

using namespace SourceKit;
using namespace sourcekitd;

void sourcekitd::postNotification(sourcekitd_response_t Notification) {
  // TODO
}

namespace {
/// Associates sourcekitd_uid_t to a UIdent.
class SKUIDToUIDMap {
  typedef llvm::DenseMap<void *, UIdent> MapTy;
  MapTy Map;
  WorkQueue Queue{ WorkQueue::Dequeuing::Concurrent, "UIDMap" };

public:
  UIdent get(sourcekitd_uid_t SKDUID);
  void set(sourcekitd_uid_t SKDUID, UIdent UID);
};
}

static SKUIDToUIDMap UIDMap;

sourcekitd_uid_t sourcekitd::SKDUIDFromUIdent(UIdent UID) {
  // TODO
}

UIdent sourcekitd::UIdentFromSKDUID(sourcekitd_uid_t SKDUID) {
  // TODO
}

std::string sourcekitd::getRuntimeLibPath() {
  return "TODO";
  // TODO
}

int main(int argc, const char *argv[]) {
  llvm::install_fatal_error_handler(fatal_error_handler, 0);
  sourcekitd::enableLogging("sourcekit-serv");
  sourcekitd::initialize();

  // TODO
  return 0;
}

UIdent SKUIDToUIDMap::get(sourcekitd_uid_t SKDUID) {
  UIdent UID;
  Queue.dispatchSync([&]{
    MapTy::iterator It = Map.find(SKDUID);
    if (It != Map.end())
      UID = It->second;
  });

  return UID;
}

void SKUIDToUIDMap::set(sourcekitd_uid_t SKDUID, UIdent UID) {
  Queue.dispatchBarrier([=]{
    this->Map[SKDUID] = UID;
  });
}
