# variantkey is a header-only library: every function is "static inline", so
# there is no object code to build and no symbol to export. An INTERFACE library
# models that and propagates the include directory to whoever links it.
add_library (variantkey INTERFACE)
target_include_directories (variantkey INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# The headers are the only installable artifacts. Without an install rule the
# generated install_manifest.txt never exists and the uninstall target aborts.
include(GNUInstallDirs)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/binsearch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/esid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/genoref.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hex.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nrvk.h
    ${CMAKE_CURRENT_SOURCE_DIR}/regionkey.h
    ${CMAKE_CURRENT_SOURCE_DIR}/rsidvar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/set.h
    ${CMAKE_CURRENT_SOURCE_DIR}/variantkey.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/variantkey)
