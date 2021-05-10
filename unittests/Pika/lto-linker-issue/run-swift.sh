#!/bin/bash

SDK_PATH=$(xcrun --sdk macosx  --show-sdk-path)
mkdir -p build

INTEROP_FLAG=
if [ "$1" == "disable-interop" ]
then
  INTEROP_FLAG='-Xfrontend -disable-objc-interop'
elif [ "$1" == "objc-only" ]
then
  INTEROP_FLAG='-Xfrontend -enable-swift-metadata-for-objc-only'
fi

BRIDGING_FLAG=
if [ "$2" == "bridging-header" ]
then
  BRIDGING_FLAG='-import-objc-header SwiftCallObjC-Bridging-Header.h'
else
  BRIDGING_FLAG='-DUSE_MODULE -I .'
fi

DIRECT_FLAG=
if [ "$3" == "enable-monolto" ]
then
  DIRECT_FLAG='-emit-bc'
fi

retcode=0
$PIKA_TOOLCHAIN/bin/swiftc -incremental -module-name FBComposerLiteRootViewController -O -whole-module-optimization \
-enforce-exclusivity=checked FBComposerLiteRootViewController.swift \
-sdk ${SDK_PATH} $INTEROP_FLAG $BRIDGING_FLAG \
-target x86_64-apple-macosx10.15 \
-module-cache-path ModuleCache.noindex -Xfrontend -serialize-debugging-options -swift-version 5 \
-c -num-threads 8 \
-emit-objc-header -emit-objc-header-path build/FBComposerLiteRootViewController-Swift.h \
-parseable-output -serialize-diagnostics -emit-dependencies -emit-module \
-emit-module-path build/FBComposerLiteRootViewController.swiftmodule \
$DIRECT_FLAG -save-temps -o FBComposerLiteRootViewController.o -parse-as-library

error=$?
if [[ $error -ne 0 ]]
then
  retcode=1
fi
file FBComposerLiteRootViewController.o
exit $retcode
