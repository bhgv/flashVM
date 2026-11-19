# Copyright (c) 2026 bhgv
# EN: Self-contained original Inferno Styx libraries, optional hosted Linux64 client/server and portable wire codec.
# RU: Самостоятельные исходные библиотеки Styx Inferno, необязательные Linux64 client/server и переносимый wire codec.
cmake_minimum_required(VERSION 3.16)
project(Styx VERSION 0.1.0 LANGUAGES C)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(STYX_HOSTED "Build original Linux64 lib9, server, client and PHY adapters" ON)
add_library(styx_codec STATIC vendor/inferno/lib9/convM2S.c vendor/inferno/lib9/convS2M.c
    vendor/inferno/lib9/convD2M.c vendor/inferno/lib9/convM2D.c)
target_include_directories(styx_codec PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/codec")
set_target_properties(styx_codec PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO EXPORT_NAME codec)
add_library(Styx::codec ALIAS styx_codec)
add_library(styx_wire STATIC protocol/srecord.c protocol/sstream.c)
target_include_directories(styx_wire PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
set_target_properties(styx_wire PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO EXPORT_NAME wire)
add_library(Styx::wire ALIAS styx_wire)
set(styx_targets styx_codec styx_wire)
if(STYX_HOSTED)
    include(cmake/Hosted.cmake)
endif()
if(BUILD_TESTING AND STYX_HOSTED)
    add_subdirectory(examples)
    foreach(original native64 stream session directories lifecycle fid-context)
        add_executable(styx-original-${original} "${src}/tools/libstyx/tests/${original}.c")
        target_include_directories(styx-original-${original} PRIVATE "${src}/tools/libstyx")
        target_link_libraries(styx-original-${original} PRIVATE Styx::server)
        add_test(NAME styx-original-${original} COMMAND styx-original-${original})
        set_tests_properties(styx-original-${original} PROPERTIES TIMEOUT 20)
    endforeach()
endif()
if(BUILD_TESTING)
    add_executable(styx-codec-test examples/codec.c)
    target_link_libraries(styx-codec-test PRIVATE Styx::codec)
    add_test(NAME styx-portable-codec COMMAND styx-codec-test)
endif()
install(TARGETS ${styx_targets} EXPORT StyxTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT StyxTargets NAMESPACE Styx:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Styx)
# Relocatable package discovery and an explicit same-minor source compatibility policy.
configure_package_config_file(cmake/StyxConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/StyxConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Styx")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/StyxConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/StyxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/StyxConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Styx")
install(FILES README.md LICENSE.bhgv SOURCES.jsonl vendor/UPSTREAM.json DESTINATION ${CMAKE_INSTALL_DATADIR}/styx)
install(DIRECTORY vendor/inferno/lib/legal DESTINATION ${CMAKE_INSTALL_DATADIR}/styx)

# Install a self-contained offline documentation tree, including source link targets.
install(FILES README.ru.md view_docs.py CMakeLists.txt .clang-format DESTINATION ${CMAKE_INSTALL_DATADIR}/styx)
install(DIRECTORY docs examples include codec client server remote_phy walk vendor protocol ports support cmake tests DESTINATION ${CMAKE_INSTALL_DATADIR}/styx
    PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
