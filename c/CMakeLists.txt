# CMake 4.0 removed the compatibility with "cmake_minimum_required" values below
# 3.5; 3.10 keeps the project buildable with both CMake 3.x and 4.x.
cmake_minimum_required(VERSION 3.10)
project(variantkey)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_definitions(-DCMAKE)

# enable testing
enable_testing()

# Extract version from the VERSION file
file(STRINGS ../VERSION VERSION_FILE_CONTENT)
file(STRINGS ../RELEASE PROJECT_RELEASE)
string(REPLACE "." ";" VERSION_FILE_PARTS ${VERSION_FILE_CONTENT})
list(GET VERSION_FILE_PARTS 0 PROJECT_VERSION_MAJOR)
list(GET VERSION_FILE_PARTS 1 PROJECT_VERSION_MINOR)
list(GET VERSION_FILE_PARTS 2 PROJECT_VERSION_PATCH)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${PROJECT_RELEASE}")

message(STATUS "PROJECT_VERSION='${PROJECT_VERSION}'")
message(STATUS "Generator - ${CMAKE_GENERATOR}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# The language standard is set by CMAKE_C_STANDARD above and must not be
# repeated here: CMake appends its own "-std=" flag after CMAKE_C_FLAGS.
#
# These warning and optimisation flags are understood by both gcc and clang, so
# they are not gated on the compiler.
#
# "-Wno-strict-prototypes" is required for clang, which enables that check under
# -Wextra while gcc does not. It only fires in the test files imported from the
# binsearch repository, which must not be edited here.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D VERSION='\"${PROJECT_VERSION}\"' -pedantic -Wall -Wextra -Wno-strict-prototypes -Wcast-align -Wundef -Wformat -Wformat-security -Wshadow")

# "-s" strips the symbols, so it belongs to the Release build only.
set(CMAKE_C_FLAGS_RELEASE   "-O3 -s -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG     "-O0 -g3")
set(CMAKE_C_FLAGS_COVERAGE  "-O0 -g3 --coverage")

# variantkey is header-only (see src/variantkey/CMakeLists.txt), so there is no
# shared object to link with "--coverage". Coverage instrumentation reaches the
# headers through each test executable's own CMAKE_C_FLAGS_COVERAGE.

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   set(OS "Mac OS X")
   # MACOSX_RPATH
   cmake_policy(SET CMP0042 NEW)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
   # Linux specific code
   # Default install path on Linux
   set(OS "Linux")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   # Windows specific code
   set(OS "Windows")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Common package settings
set(CPACK_PACKAGE_VENDOR "Tecnick.com")
set(CPACK_PACKAGE_CONTACT "info@tecnick.com")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_COMPONENTS_IGNORE_GROUPS TRUE)
set(CPACK_DEB_COMPONENT_INSTALL TRUE)

# Add subdirectories
add_subdirectory(src/variantkey)
add_subdirectory(test)
add_subdirectory(vk)
add_subdirectory(test/rsidvar_bench)

# Build Documentation.
# The doc target is defined when Doxygen is installed. Whether the docs are
# generated is decided by the VH_BUILD_DOXYGEN switch in the Makefile.
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_subdirectory(doc)
else(DOXYGEN_FOUND)
    message(STATUS "Doxygen not found. Documentation will not be built.")
endif(DOXYGEN_FOUND)

message(STATUS "Install ${CMAKE_INSTALL_PREFIX}")

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
