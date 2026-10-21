# Cross-compile the independent C89/C++98 core with an explicitly selected Zig Linux/musl target.
# No host SDK discovery or target execution is implied; supply a target-built SDK for adapters.
# STRING preserves a relative CLI spelling so validation cannot silently absolutize it.
set(FVM_ZIG_EXECUTABLE "" CACHE STRING "Absolute path to an installed Zig executable")
set(FVM_ZIG_TARGET "" CACHE STRING "Explicit Linux/musl target: aarch64, riscv64 or x86_64")
if(NOT FVM_ZIG_EXECUTABLE OR NOT IS_ABSOLUTE "${FVM_ZIG_EXECUTABLE}" OR
   NOT EXISTS "${FVM_ZIG_EXECUTABLE}" OR IS_DIRECTORY "${FVM_ZIG_EXECUTABLE}")
  message(FATAL_ERROR "FVM_ZIG_EXECUTABLE must name an existing absolute executable file")
endif()
if(FVM_ZIG_TARGET STREQUAL "aarch64-linux-musl")
  set(FVM_ZIG_PROCESSOR aarch64)
elseif(FVM_ZIG_TARGET STREQUAL "riscv64-linux-musl")
  set(FVM_ZIG_PROCESSOR riscv64)
elseif(FVM_ZIG_TARGET STREQUAL "x86_64-linux-musl")
  set(FVM_ZIG_PROCESSOR x86_64)
else()
  message(FATAL_ERROR "FVM_ZIG_TARGET must be aarch64-linux-musl, riscv64-linux-musl or x86_64-linux-musl")
endif()
# A cached compiler or initialized flags must never silently retain a previous ABI.
# A separate build directory is required even when only the compiler installation changes.
foreach(FVM_ZIG_INPUT IN ITEMS FVM_ZIG_EXECUTABLE FVM_ZIG_TARGET)
  if(DEFINED ${FVM_ZIG_INPUT}_CONFIGURED AND
     NOT "${${FVM_ZIG_INPUT}_CONFIGURED}" STREQUAL "${${FVM_ZIG_INPUT}}")
    message(FATAL_ERROR "${FVM_ZIG_INPUT} changed: use a fresh build directory")
  endif()
  set(${FVM_ZIG_INPUT}_CONFIGURED "${${FVM_ZIG_INPUT}}" CACHE INTERNAL "Configured Zig identity")
endforeach()
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR "${FVM_ZIG_PROCESSOR}")
# CMake preserves the subcommand as a mandatory compiler argument, including paths with spaces.
set(CMAKE_C_COMPILER "${FVM_ZIG_EXECUTABLE}" cc)
set(CMAKE_CXX_COMPILER "${FVM_ZIG_EXECUTABLE}" c++)
set(CMAKE_AR "${FVM_ZIG_EXECUTABLE}" CACHE FILEPATH "Zig archiver driver" FORCE)
# Zig uses an ar subcommand; rules avoid a shell wrapper or an assumed host binutils archive format.
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> ar qc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> ar q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> ranlib <TARGET>")
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> ar qc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> ar q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> ranlib <TARGET>")
set(CMAKE_C_FLAGS_INIT "-target ${FVM_ZIG_TARGET}")
set(CMAKE_CXX_FLAGS_INIT "-target ${FVM_ZIG_TARGET}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
# Compiler identification remains a compile check; final selected executables must still link.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
# Repeat explicit inputs in nested compiler checks instead of accidentally probing host tools.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES FVM_ZIG_EXECUTABLE FVM_ZIG_TARGET)
# Programs execute on the build host. Headers/libraries/packages belong to the selected target.
# Consumers using additional SDKs should supply CMAKE_FIND_ROOT_PATH and target package prefixes.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
