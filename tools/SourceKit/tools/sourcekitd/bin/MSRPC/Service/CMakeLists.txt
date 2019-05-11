if (SOURCEKIT_USE_MSRPC)
  add_sourcekit_msrpc_service(SourceKitService sourcekitd
    MSRPCService.cpp
    LINK_LIBS SourceKitSwiftLang sourcekitdAPI
    LLVM_LINK_COMPONENTS support coverage
  )
endif()
