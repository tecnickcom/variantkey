set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/cmd)

# NOTE: variantkey is an INTERFACE library, so it propagates its own include
# directory through target_link_libraries; there is nothing to link against and
# nothing to copy here.
add_executable(vkbin vkbin.c)
target_link_libraries(vkbin variantkey)

# --- PACKAGING ---

install(TARGETS "vkbin" DESTINATION "bin" COMPONENT "vkbin")
install(FILES "../../README.md" DESTINATION "share/doc/vkbin" COMPONENT "vkbin")
install(FILES "../../VERSION" DESTINATION "share/doc/vkbin" COMPONENT "vkbin")
install(FILES "../../RELEASE" DESTINATION "share/doc/vkbin" COMPONENT "vkbin")
install(FILES "../../LICENSE" DESTINATION "share/doc/vkbin" COMPONENT "vkbin" RENAME copyright)

set(CPACK_PACKAGE_NAME "vkbin")
set(CPACK_PACKAGE_DESCRIPTION "VariantKey binary lookup table builder")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VariantKey Binary Lookup Table Builder Command Line Application")

include(CPack)
