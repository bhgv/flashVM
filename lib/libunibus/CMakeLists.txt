# Copyright (c) 2026 bhgv
# EN: Independent allocation-free wire library and optional native per-tag queues; no Styx or VM dependency.
# RU: Независимый wire codec без выделений и необязательные нативные очереди тегов; без Styx и VM.
cmake_minimum_required(VERSION 3.16)
project(libunibus VERSION 0.1.0 LANGUAGES C)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
option(UNIBUS_QUEUES "Build pthread-backed per-tag ordered queues" ON)
add_library(unibus_wire STATIC protocol/srecord.c protocol/sstream.c protocol/sbindings.c protocol/scontrol.c
    protocol/sslot.c protocol/sadapt.c protocol/smeasure.c protocol/smedia.c protocol/order/prefix.c
    protocol/order/packet.c protocol/order/stream.c protocol/order/control.c)
target_include_directories(unibus_wire PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
set_target_properties(unibus_wire PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO EXPORT_NAME wire)
add_library(Unibus::wire ALIAS unibus_wire)
set(unibus_targets unibus_wire)
if(UNIBUS_QUEUES)
    find_package(Threads REQUIRED)
    add_library(unibus_queues STATIC order/life.c order/receive.c order/inbox.c order/send.c order/rollover.c)
    target_include_directories(unibus_queues PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/order>" "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/unibus>")
    target_link_libraries(unibus_queues PUBLIC unibus_wire Threads::Threads)
    set_target_properties(unibus_queues PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES EXPORT_NAME queues)
    add_library(Unibus::queues ALIAS unibus_queues)
    list(APPEND unibus_targets unibus_queues)
    install(FILES order/order.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/unibus)
endif()
if(BUILD_TESTING)
    add_subdirectory(examples)
endif()
# The implementation stays C; other-language example/consumer targets are exempt.
include(cmake/OnlyC.cmake)
require_c_libraries(${unibus_targets})
install(TARGETS ${unibus_targets} EXPORT UnibusTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT UnibusTargets NAMESPACE Unibus:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Unibus)
# Relocatable package discovery and an explicit same-minor source compatibility policy.
configure_package_config_file(cmake/UnibusConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/UnibusConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Unibus")
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/UnibusConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/UnibusConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/UnibusConfigVersion.cmake" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Unibus")
install(FILES README.md LICENSE.bhgv SOURCES.jsonl DESTINATION ${CMAKE_INSTALL_DATADIR}/libunibus)

# Install a self-contained offline documentation tree, including source link targets.
install(FILES README.ru.md view_docs.py CMakeLists.txt .clang-format DESTINATION ${CMAKE_INSTALL_DATADIR}/libunibus)
install(DIRECTORY docs examples include protocol order cmake tests DESTINATION ${CMAKE_INSTALL_DATADIR}/libunibus
    PATTERN "__pycache__" EXCLUDE PATTERN "*.pyc" EXCLUDE)
