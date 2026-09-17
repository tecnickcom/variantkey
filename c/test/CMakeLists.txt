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

# test_vkbin drives the vkbin executable over a TSV fixture, so both the fixture
# and the path of the built tool must reach it.
file (COPY "data/vkbin.tsv" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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

# The find_many page requests need MADV_WILLNEED, which is POSIX and so is not
# declared in the strict ISO C build the glob above uses: there the prefetch
# code is not generated at all. These two targets build the same test with it
# declared, as upstream does, which is the configuration an ordinary caller
# compiles. A small BINSEARCH_PROBE_EVERY makes the test cross a probe interval,
# and the two thresholds cover both answers the probe can give.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_binsearch_many.c")
  SMOKE_TEST (test_binsearch_prefetch test_binsearch_many.c variantkey)
  target_compile_definitions (test_binsearch_prefetch PRIVATE _DEFAULT_SOURCE BINSEARCH_PROBE_EVERY=2 BINSEARCH_SLOW_NS=1)

  SMOKE_TEST (test_binsearch_prefetch_auto test_binsearch_many.c variantkey)
  target_compile_definitions (test_binsearch_prefetch_auto PRIVATE _DEFAULT_SOURCE BINSEARCH_PROBE_EVERY=2)
endif()

SMOKE_TEST (test_esid test_esid.c variantkey)
SMOKE_TEST (test_example test_example.c variantkey)
SMOKE_TEST (test_genoref test_genoref.c variantkey)
SMOKE_TEST (test_hex test_hex.c variantkey)
SMOKE_TEST (test_nrvk test_nrvk.c variantkey)
SMOKE_TEST (test_regionkey test_regionkey.c variantkey)
SMOKE_TEST (test_rsidvar test_rsidvar.c variantkey)
SMOKE_TEST (test_set test_set.c variantkey)
SMOKE_TEST (test_variantkey test_variantkey.c variantkey)

SMOKE_TEST (test_vkbin test_vkbin.c variantkey)
add_dependencies(test_vkbin vkbin)
target_compile_definitions(test_vkbin PRIVATE
  VKBIN_PATH="$<TARGET_FILE:vkbin>"
  VKBIN_DATA="${CMAKE_CURRENT_BINARY_DIR}/vkbin.tsv")

# test_vkbin_unit compiles the tool itself, so it reaches the internal functions
# and the error paths that cannot be provoked through the command line. It
# writes the inputs of its cases, so it needs no fixture.
SMOKE_TEST (test_vkbin_unit test_vkbin_unit.c variantkey)
