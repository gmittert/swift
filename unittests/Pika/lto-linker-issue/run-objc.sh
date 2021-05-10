#!/bin/bash

# add -v -save-temps -emit-llvm, remove -g -gmodules
# fmodules doesn't work with -save-temps

SDK_PATH=$(xcrun --sdk macosx --show-sdk-path)

DIRECT_FLAG=
if [ "$1" == "objc-direct" ]
then
  DIRECT_FLAG='-DENABLE_OBJC_DIRECT'
fi

PIKA_FLAGS='-fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -std=gnu11 -fobjc-arc -fmodules -fmodules-cache-path=ModuleCache.noindex -Wnon-modular-include-in-framework-module -Werror=non-modular-include-in-framework-module -Wno-trigraphs -fpascal-strings -Os -fno-common -fasm-blocks -fstrict-aliasing -Wprotocol -Wdeprecated-declarations -fvisibility=hidden -Wno-sign-conversion -Winfinite-recursion -Wcomma -Wblock-capture-autoreleasing -Wstrict-prototypes -Wno-semicolon-before-method-body -Wunguarded-availability -fobjc-abi-version=2 -fobjc-legacy-dispatch -I build'

$PIKA_TOOLCHAIN/bin/clang -x objective-c -arch x86_64 $PIKA_FLAGS $DIRECT_FLAG \
-DNS_BLOCK_ASSERTIONS=1 -DOBJC_OLD_DISPATCH_PROTOTYPES=0 \
-isysroot ${SDK_PATH} \
-c FBComposerLiteExtensionController.mm	-flto=thin

$PIKA_TOOLCHAIN/bin/clang -x objective-c -arch x86_64 $PIKA_FLAGS $DIRECT_FLAG \
-DNS_BLOCK_ASSERTIONS=1 -DOBJC_OLD_DISPATCH_PROTOTYPES=0 \
-isysroot ${SDK_PATH} \
-c FBComposerLiteUIRootSwiftPlugin.m -flto=thin
