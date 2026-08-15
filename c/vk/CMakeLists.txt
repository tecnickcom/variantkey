set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/cmd)

# NOTE: variantkey is an INTERFACE library, so it propagates its own include
# directory through target_link_libraries; there is nothing to link against and
# nothing to copy here.
add_executable(vk vk.c)
target_link_libraries(vk variantkey)

# --- PACKAGING ---

install(TARGETS "vk" DESTINATION "bin" COMPONENT "vk")
install(FILES "../../README.md" DESTINATION "share/doc/vk" COMPONENT "vk")
install(FILES "../../VERSION" DESTINATION "share/doc/vk" COMPONENT "vk")
install(FILES "../../RELEASE" DESTINATION "share/doc/vk" COMPONENT "vk")
install(FILES "../../LICENSE" DESTINATION "share/doc/vk" COMPONENT "vk" RENAME copyright)

set(CPACK_PACKAGE_NAME "vk")
set(CPACK_PACKAGE_DESCRIPTION "VariantKey CLI encoder")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VariantKey Fast Encoder Command Line Application")

include(CPack)
