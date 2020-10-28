add_subdirectory(swift)

swift_install_in_component(DIRECTORY "${SWIFT_MAIN_INCLUDE_DIR}/"
                           DESTINATION "include"
                           COMPONENT swift-headers)

swift_install_in_component(DIRECTORY "${SWIFT_INCLUDE_DIR}/"
                           DESTINATION "include"
                           COMPONENT swift-headers)

if (TARGET swift-syntax-generated-headers)
  add_dependencies(swift-headers swift-syntax-generated-headers)
endif()
