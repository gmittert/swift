#!/bin/bash

if [ -z $PIKA_TOOLCHAIN ]; then
    echo "Please provide the path to your pika toolchain via the PIKA_TOOLCHAIN environment variable!"
    echo "  clang should be under \$PIKA_TOOLCHAIN/bin"
    exit 1
fi

LIBRARY_PATH=${PIKA_TOOLCHAIN}/lib/swift/macosx

run_test() {
  run_fail=0
  if [[ "$1" == "disable-interop" || "$3" == "objc-direct" ]]
  then
    run_fail=1
  fi

  ./run-swift.sh $1 $2 $3 &> tmp.log
  error=$?
  # Expect error with disable-interop
  if [[ $error -ne 0 && $run_fail -ne 1 ]]
  then
    echo "Error: swift compilation failed"
    retcode=1
  fi
  ./run-objc.sh $3

  if [ $run_fail -ne 1 ]
  then
    ./link.sh &> link.log
    grep -E 's32FBComposerLiteRootViewControllerAACN|FBSwiftComposerLiteRootViewController|atoms:|assignAtomAddresses' link.log

    codesign -f -s - mytest

    echo "  run pika linker"
    /usr/bin/env DYLD_LIBRARY_PATH=${LIBRARY_PATH} LD_LIBRARY_PATH=${LIBRARY_PATH} SIMCTL_CHILD_DYLD_LIBRARY_PATH=${LIBRARY_PATH} ./mytest &> tmp.log
    error=$?
    if [ $error -ne 0 ]
    then
       echo "Error: unexpected run result"
       retcode=1
    fi
    cp mytest mytest-pika

    ./link-sys.sh
    codesign -f -s - mytest

    echo "  run system linker"
    /usr/bin/env DYLD_LIBRARY_PATH=${LIBRARY_PATH} LD_LIBRARY_PATH=${LIBRARY_PATH} SIMCTL_CHILD_DYLD_LIBRARY_PATH=${LIBRARY_PATH} ./mytest &> tmp.log
  fi

  #clean
  rm -rf mytest* *.a *.o *.ll *.bc *.log *.d *.dia build ModuleCache.noindex
}

retcode=0
echo "enable-monolto"
run_test empty bridging-header enable-monolto
echo "no monolto"
run_test empty bridging-header
