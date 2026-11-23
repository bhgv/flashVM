# Copyright (c) 2026 bhgv
# EN: Independent C RPC codec and callback endpoint over installed communication libraries.
# RU: Независимый C-кодек RPC и колбэки поверх установленных библиотек связи.
cmake_minimum_required(VERSION 3.16)
project(StyxRpc VERSION 0.1.0 LANGUAGES C)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Unibus CONFIG REQUIRED)
option(STYXRPC_EXAMPLES "Build the hosted Styx callback device" OFF)
add_library(styxrpc STATIC src/rpc.c src/vfs.c protocol/call_record.c protocol/call_scope.c
    protocol/load_profile.c protocol/ref_pair_record.c protocol/scalar_record.c
    protocol/scalar_verify.c protocol/tuple_record.c protocol/utf8.c)
target_link_libraries(styxrpc PUBLIC Unibus::wire)
target_include_directories(styxrpc PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>" PRIVATE protocol)
set_target_properties(styxrpc PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO EXPORT_NAME rpc)
add_library(StyxRpc::rpc ALIAS styxrpc)
if(STYXRPC_EXAMPLES)
    find_package(Styx CONFIG REQUIRED)
    add_executable(styxrpc-device examples/device.c)
    target_link_libraries(styxrpc-device PRIVATE styxrpc Styx::phy_server)
    add_executable(styxrpc-client examples/client.c)
    target_link_libraries(styxrpc-client PRIVATE styxrpc Styx::client)
    if(BUILD_TESTING)
        find_package(Python3 QUIET COMPONENTS Interpreter)
        if(Python3_Interpreter_FOUND)
            add_test(NAME styxrpc-native-socket COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/examples/socketpair.py"
                "$<TARGET_FILE:styxrpc-device>" "$<TARGET_FILE:styxrpc-client>")
        endif()
    endif()
endif()
if(BUILD_TESTING)
    add_executable(styxrpc-test tests/scalar.c)
    target_include_directories(styxrpc-test PRIVATE protocol)
    target_link_libraries(styxrpc-test PRIVATE styxrpc)
    add_test(NAME styxrpc-scalar COMMAND styxrpc-test)
endif()
install(TARGETS styxrpc EXPORT StyxRpcTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT StyxRpcTargets NAMESPACE StyxRpc:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StyxRpc)
configure_package_config_file(cmake/StyxRpcConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/StyxRpcConfig.cmake" INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StyxRpc)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/StyxRpcConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/StyxRpcConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/StyxRpcConfigVersion.cmake" DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/StyxRpc)
