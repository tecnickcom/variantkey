# Tests

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# NOTE: variantkey is an INTERFACE library (see src/variantkey/CMakeLists.txt),
# so it propagates its own include directory through target_link_libraries and
# has no binary directory to link against. Only the current binary directory is
# needed here, for the test data files copied below.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#define macro to simplify adding tests
macro (do_test arg)
  add_test (${arg} ${TARGET_SYSTEM_EMULATOR} ${arg}${CMAKE_EXECUTABLE_SUFFIX})
endmacro ()

# create a smoke test
function(SMOKE_TEST test_name test_file dependencies)
  add_executable(${test_name} ${test_file})
  # Link the executable to the libraries
  target_link_libraries(${test_name} ${dependencies})
  # run test
  do_test (${test_name})
endfunction(SMOKE_TEST)

file(GLOB TEST_BIN_FILES "data/*.bin")
file (COPY ${TEST_BIN_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# The binsearch tests are imported from the upstream binsearch repository by the
# "make binsearch" target, so they are registered by glob: a new upstream test
# file is picked up without editing this file. The glob is evaluated at
# configure time, which is enough because the test target always reconfigures
# the build from scratch.
file(GLOB BINSEARCH_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "test_binsearch*.c")
foreach(binsearch_test_file ${BINSEARCH_TEST_FILES})
  get_filename_component(binsearch_test_name ${binsearch_test_file} NAME_WE)
  SMOKE_TEST (${binsearch_test_name} ${binsearch_test_file} variantkey)
endforeach()

SMOKE_TEST (test_esid test_esid.c variantkey)
SMOKE_TEST (test_example test_example.c variantkey)
SMOKE_TEST (test_genoref test_genoref.c variantkey)
SMOKE_TEST (test_hex test_hex.c variantkey)
SMOKE_TEST (test_nrvk test_nrvk.c variantkey)
SMOKE_TEST (test_regionkey test_regionkey.c variantkey)
SMOKE_TEST (test_rsidvar test_rsidvar.c variantkey)
SMOKE_TEST (test_set test_set.c variantkey)
SMOKE_TEST (test_variantkey test_variantkey.c variantkey)
