# libunibus: embedding and installed SDK

Copyright (c) 2026 bhgv

[README](../README.md) · [Introduction](introduction.md) · [Tutorial](tutorial.md) · [API](index.md)

## Choosing a route

An installed SDK separates library builds from application builds. A copied source directory lets you customize adapters or select an MCU toolchain. Both routes use package-owned sources without DisVM, FlashVM or the sibling library. Native acceptance is Linux amd64, not every physical device or OS.

## 1. Build and install the library

```sh
cmake -S . -B build-sdk -DUNIBUS_QUEUES=OFF -DBUILD_TESTING=OFF
cmake --build build-sdk -j
cmake --install build-sdk --prefix "$PWD/sdk"
```

Run from the copied package root. OFF selects the minimal profile; use ON for native layers. BUILD_TESTING=OFF excludes teaching programs and their test-only dependencies. Hosted Styx still uses Python during lib9 source selection.

## 2. Consume from an independent application

```sh
mkdir ../device-example
cp examples/nested.c ../device-example/main.c
```

Save the following as ../device-example/CMakeLists.txt. main.c is a complete package example, not a missing placeholder.

```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
find_package(Unibus 0.1 CONFIG REQUIRED COMPONENTS wire)
add_executable(device main.c)
target_link_libraries(device PRIVATE Unibus::wire)
set_target_properties(device PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
```

```sh
cmake -S ../device-example -B ../device-example/build -DCMAKE_PREFIX_PATH="$PWD/sdk"
cmake --build ../device-example/build
../device-example/build/device
```

CMAKE_PREFIX_PATH names the installed prefix, not its include directory or original checkout. After moving the whole SDK, point to its new location. Targets supply include paths and transitive libraries; do not hardcode old absolute paths. CMAKE_INSTALL_INCLUDEDIR is supported, including api/include. C++ applications enable CXX in project and may select CXX_STANDARD 98. Styx client C linkage preserves the C ABI for C++ calls.

## Version and components

Package version is 0.1.0. Requesting 0.1 accepts a compatible installed version within that minor line while respecting the requested minimum. EXACT requires an exact version; another minor/major line is not automatically accepted. This is an SDK discovery policy, not a wire version or a promise about every future ABI.

| Component | Target | Availability |
| --- | --- | --- |
| `wire` | `Unibus::wire` | `always` |
| `queues` | `Unibus::queues` | `UNIBUS_QUEUES=ON` |

```cmake
find_package(Unibus 0.1 CONFIG REQUIRED
    COMPONENTS wire OPTIONAL_COMPONENTS queues)
if(Unibus_queues_FOUND)
    target_link_libraries(device PRIVATE Unibus::queues)
endif()
```

This is an alternative to the earlier find_package call. A required component absent from the build stops configuration with a reason. An unavailable optional component leaves the minimal layer usable. Unknown required names are rejected too.

## 3. Embed a source copy

```cmake
cmake_minimum_required(VERSION 3.16)
project(Device C)
set(UNIBUS_QUEUES OFF CACHE BOOL "Minimal communication layer")
set(BUILD_TESTING OFF CACHE BOOL "Build the application without library examples")
add_subdirectory(libunibus-source)
add_executable(device main.c)
target_link_libraries(device PRIVATE Unibus::wire)
```

This is a complete application CMakeLists with a copied package directory alongside it. Cache options belong to the outer build; coordinate BUILD_TESTING with an existing project. Discovery options cannot change an already installed build profile: adding native components requires rebuilding/reinstalling the library.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Package not found | CMAKE_PREFIX_PATH and completed installation |
| Version incompatible | Requested version, EXACT and installed minor line |
| Component unavailable | Library build option and exact component name/case |
| C++ undefined reference | Matching headers/library and the exported target |
| Header missing after moving SDK | Move the whole prefix; remove hardcoded old include paths |
