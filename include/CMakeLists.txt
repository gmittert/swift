add_subdirectory(swift)

swift_install_in_component(DIRECTORY "${SWIFT_MAIN_INCLUDE_DIR}/"
                           DESTINATION "include"
                           COMPONENT swift-headers)

swift_install_in_component(DIRECTORY "${SWIFT_INCLUDE_DIR}/"
                           DESTINATION "include"
                           COMPONENT swift-headers)

swift_install_in_component(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../stdlib/public/SwiftShims"
                           DESTINATION "stdlib/public"
                           COMPONENT swift-headers)

if (TARGET swift-syntax-generated-headers)
  add_dependencies(swift-headers swift-syntax-generated-headers)
endif()
