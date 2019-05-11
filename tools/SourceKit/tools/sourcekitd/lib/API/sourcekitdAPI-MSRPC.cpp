//===--- sourcekitdAPI-MSRPC.cpp --------------------------------------------===//
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

#include "DictionaryKeys.h"
#include "sourcekitd/CodeCompletionResultsArray.h"
#include "sourcekitd/DocStructureArray.h"
#include "sourcekitd/DocSupportAnnotationArray.h"
#include "sourcekitd/TokenAnnotationsArray.h"
#include "sourcekitd/ExpressionTypeArray.h"
#include "sourcekitd/RawData.h"
#include "sourcekitd/RequestResponsePrinterBase.h"
#include "SourceKit/Support/UIdent.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MemoryBuffer.h"
#include <vector>

using namespace SourceKit;
using namespace sourcekitd;
using llvm::ArrayRef;
using llvm::StringRef;
using llvm::raw_ostream;

namespace {

template <typename ImplClass, typename RetTy = void>
class SKDObjectVisitor {
public:
  typedef std::vector<std::pair<UIdent, sourcekitd_object_t>> DictMap;

  static bool compKeys(const std::pair<UIdent, sourcekitd_object_t> &LHS,
                       const std::pair<UIdent, sourcekitd_object_t> &RHS) {
    return sourcekitd::compareDictKeys(LHS.first, RHS.first);
  }

  // TODO
  // RetTy visit(sourcekitd_object_t Obj) { }
};

class SKDObjectPrinter : public SKDObjectVisitor<SKDObjectPrinter>,
                         public RequestResponsePrinterBase<SKDObjectPrinter,
                                                           sourcekitd_object_t> {
public:
  SKDObjectPrinter(raw_ostream &OS, unsigned Indent = 0)
    : RequestResponsePrinterBase(OS, Indent) { }
};

} // anonymous namespace

void sourcekitd::printRequestObject(sourcekitd_object_t Obj, raw_ostream &OS) {
  // TODO
}

//===----------------------------------------------------------------------===//
// Internal API
//===----------------------------------------------------------------------===//

ResponseBuilder::ResponseBuilder() {
  // TODO 
}

ResponseBuilder::~ResponseBuilder() {
  // TODO
}

ResponseBuilder::ResponseBuilder(const ResponseBuilder &Other) {
  // TODO 
}

ResponseBuilder &ResponseBuilder::operator =(const ResponseBuilder &Other) {
  // TODO
  return *this;
}

// ResponseBuilder::Dictionary ResponseBuilder::getDictionary() { TODO }

// TODO
// sourcekitd_response_t ResponseBuilder::createResponse() { TODO }

void ResponseBuilder::Dictionary::set(UIdent Key, SourceKit::UIdent UID) {
  set(Key, SKDUIDFromUIdent(UID));
}

void ResponseBuilder::Dictionary::set(UIdent Key, sourcekitd_uid_t UID) {
  // TODO
}

void ResponseBuilder::Dictionary::set(UIdent Key, const char *Str) {
  // TODO
}

void ResponseBuilder::Dictionary::set(UIdent Key, llvm::StringRef Str) {
  llvm::SmallString<512> Buf(Str);
  // TODO
}

void ResponseBuilder::Dictionary::set(UIdent Key, const std::string &Str) {
  // TODO
}

void ResponseBuilder::Dictionary::set(UIdent Key, int64_t val) {
  // TODO
}

void ResponseBuilder::Dictionary::set(SourceKit::UIdent Key,
                                      ArrayRef<StringRef> Strs) {
  // TODO
}

void ResponseBuilder::Dictionary::set(SourceKit::UIdent Key,
                                      ArrayRef<std::string> Strs) {
  // TODO
}

void ResponseBuilder::Dictionary::setBool(UIdent Key, bool val) {
  // TODO
}

// TODO
// ResponseBuilder::Array ResponseBuilder::Dictionary::setArray(UIdent Key) { }

// TODO
// ResponseBuilder::Dictionary ResponseBuilder::Dictionary::setDictionary(UIdent Key) { }

void ResponseBuilder::Dictionary::setCustomBuffer(
      SourceKit::UIdent Key,
      CustomBufferKind Kind, std::unique_ptr<llvm::MemoryBuffer> MemBuf) {
  // TODO
}

// ResponseBuilder::Dictionary ResponseBuilder::Array::appendDictionary() { TODO }

// sourcekitd_uid_t RequestDict::getUID(UIdent Key) { TODO }

// Optional<StringRef> RequestDict::getString(UIdent Key) { TODO }

// TODO
// Optional<RequestDict> RequestDict::getDictionary(SourceKit::UIdent Key) { }

bool RequestDict::getStringArray(SourceKit::UIdent Key,
                                 llvm::SmallVectorImpl<const char *> &Arr,
                                 bool isOptional) {
  // TODO
  return false;
}

bool RequestDict::getUIDArray(SourceKit::UIdent Key,
                              llvm::SmallVectorImpl<sourcekitd_uid_t> &Arr,
                              bool isOptional) {
  // TODO
  return false;
}

bool RequestDict::dictionaryArrayApply(
    SourceKit::UIdent key, llvm::function_ref<bool(RequestDict)> applier) {
  // TODO
  return false;
}

bool RequestDict::getInt64(SourceKit::UIdent Key, int64_t &Val,
                           bool isOptional) {
  // TODO
  return false;
}

// TODO
// Optional<int64_t> RequestDict::getOptionalInt64(SourceKit::UIdent Key) { }

// TODO
// sourcekitd_response_t sourcekitd::createErrorRequestInvalid(const char *Description) { }
// TODO
// sourcekitd_response_t sourcekitd::createErrorRequestFailed(const char *Description) { }
// TODO
// sourcekitd_response_t sourcekitd::createErrorRequestInterrupted(const char *Description) { }
// TODO
// sourcekitd_response_t sourcekitd::createErrorRequestCancelled() { }

//===----------------------------------------------------------------------===//
// Public Request API
//===----------------------------------------------------------------------===//

static inline const char *strFromUID(sourcekitd_uid_t uid) {
  return UIdentFromSKDUID(uid).c_str();
}

// TODO
// sourcekitd_object_t sourcekitd_request_retain(sourcekitd_object_t object) { }

void sourcekitd_request_release(sourcekitd_object_t object) {
// TODO
}

// TODO
// sourcekitd_object_t sourcekitd_request_dictionary_create(const sourcekitd_uid_t *keys, const sourcekitd_object_t *values, size_t count) { }

void
sourcekitd_request_dictionary_set_value(sourcekitd_object_t dict,
                                        sourcekitd_uid_t key,
                                        sourcekitd_object_t value) {
// TODO
}

void sourcekitd_request_dictionary_set_string(sourcekitd_object_t dict,
                                              sourcekitd_uid_t key,
                                              const char *string) {
// TODO
}

void
sourcekitd_request_dictionary_set_stringbuf(sourcekitd_object_t dict,
                                            sourcekitd_uid_t key,
                                            const char *buf, size_t length) {
// TODO
}

void sourcekitd_request_dictionary_set_int64(sourcekitd_object_t dict,
                                             sourcekitd_uid_t key,
                                             int64_t val) {
// TODO
}

void
sourcekitd_request_dictionary_set_uid(sourcekitd_object_t dict,
                                      sourcekitd_uid_t key,
                                      sourcekitd_uid_t uid) {
// TODO
}

// TODO
// sourcekitd_object_t sourcekitd_request_array_create(const sourcekitd_object_t *objects, size_t count) { }

void
sourcekitd_request_array_set_value(sourcekitd_object_t array, size_t index,
                                   sourcekitd_object_t value) {
  // TODO
}

void
sourcekitd_request_array_set_string(sourcekitd_object_t array, size_t index,
                                    const char *string) {
  // TODO
}

void
sourcekitd_request_array_set_stringbuf(sourcekitd_object_t array, size_t index,
                                       const char *buf, size_t length) {
  // TODO
}

void
sourcekitd_request_array_set_int64(sourcekitd_object_t array, size_t index,
                                   int64_t val) {
  // TODO
}

void
sourcekitd_request_array_set_uid(sourcekitd_object_t array, size_t index,
                                 sourcekitd_uid_t uid) {
  // TODO
}

// TODO
// sourcekitd_object_t sourcekitd_request_int64_create(int64_t val) { }

// TODO
// sourcekitd_object_t sourcekitd_request_string_create(const char *string) { }

// TODO
// sourcekitd_object_t sourcekitd_request_uid_create(sourcekitd_uid_t uid) { }

//===----------------------------------------------------------------------===//
// Public Response API
//===----------------------------------------------------------------------===//

void
sourcekitd_response_dispose(sourcekitd_response_t obj) {
  // TODO
}

bool
sourcekitd_response_is_error(sourcekitd_response_t obj) {
  // TODO
  return false;
}

// TODO
// sourcekitd_error_t sourcekitd_response_error_get_kind(sourcekitd_response_t obj) { }

// TODO
// const char * sourcekitd_response_error_get_description(sourcekitd_response_t obj) { }

// TODO
// sourcekitd_variant_t sourcekitd_response_get_value(sourcekitd_response_t resp) { }

