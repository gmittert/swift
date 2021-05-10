#!/bin/bash
SDK_PATH=$(xcrun --sdk macosx  --show-sdk-path)
FRAME_PATH=${SDK_PATH}/../../Library/Frameworks

retcode=0

# create archive for .o of swift source: FBComposerLiteRootViewController.swift
rm FBComposerLiteRootViewController.a
~/local/pika11/build/RelWithDebInfo/Darwin-x86_64/toolchain/bin/llvm-ar -q FBComposerLiteRootViewController.a FBComposerLiteRootViewController.o
ld -demangle -lto_library $PIKA_TOOLCHAIN/lib/libLTO.dylib \
FBComposerLiteExtensionController.o FBComposerLiteUIRootSwiftPlugin.o FBComposerLiteRootViewController.a $PIKA_TOOLCHAIN/lib/swift/clang/lib/darwin/libclang_rt.osx.a \
-force_load $PIKA_TOOLCHAIN/lib/arc/libarclite_macosx.a -framework CoreFoundation -lobjc \
-F ${FRAME_PATH} -L . -F $PIKA_TOOLCHAIN/lib -syslibroot ${SDK_PATH} \
-lSystem -arch x86_64 -force_load $PIKA_TOOLCHAIN/lib/swift/macosx/libswiftCompatibility50.a \
-force_load $PIKA_TOOLCHAIN/lib/swift/macosx/libswiftCompatibilityDynamicReplacements.a \
-L $PIKA_TOOLCHAIN/lib/swift/macosx -L ${SDK_PATH}/usr/lib/swift -rpath $PIKA_TOOLCHAIN/lib/swift/macosx \
-rpath ${SDK_PATH}/usr/lib/swift -macosx_version_min 10.9.0 -no_objc_category_merging -rpath ${FRAME_PATH} \
-rpath $PIKA_TOOLCHAIN/lib -o mytest
error=$?
if [[ $error -ne 0 ]]
then
  retcode=1
fi

exit $retcode
