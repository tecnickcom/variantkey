set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/cmd)

# NOTE: variantkey is an INTERFACE library, so it propagates its own include
# directory through target_link_libraries; there is nothing to link against and
# nothing to copy here.
add_executable(rsidvar_bench rsidvar_bench.c)
target_link_libraries(rsidvar_bench variantkey)

add_executable(vk_csv_lookup_bench vk_csv_lookup_bench.c)
target_link_libraries(vk_csv_lookup_bench variantkey)
