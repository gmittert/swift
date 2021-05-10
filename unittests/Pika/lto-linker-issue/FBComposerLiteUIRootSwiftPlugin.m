// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "FBComposerLiteExtensionController.h"
#import "FBComposerLiteRootViewController-Swift.h"

NSObject *_Nonnull FBSwiftComposerLiteUIRootPluginHandler(FBComposerLiteExtensionController *_Nonnull controller)
{
  return [[FBSwiftComposerLiteRootViewController alloc] initWithExtensionController:controller];
}

int main() {
  FBComposerLiteExtensionController *c = [FBComposerLiteExtensionController new];
  FBSwiftComposerLiteUIRootPluginHandler(c);
  return 0;
}
