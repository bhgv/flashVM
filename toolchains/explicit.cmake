# Optional, explicit cross SDK description shared by otherwise independent projects.
# Copy this file with a detached component or supply its absolute path externally.
# Host generators (Coco/R, Python, CMake) are never searched inside the target SDK.

# Propagate SDK selections into CMake's isolated compiler/linker probe projects.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  FVM_TARGET_SYSTEM FVM_TARGET_PROCESSOR FVM_TARGET_CC FVM_TARGET_CXX
  FVM_TARGET_SYSROOT FVM_TARGET_AR FVM_TARGET_RANLIB FVM_TARGET_RC)

set(FVM_TARGET_SYSTEM "" CACHE STRING "CMake target OS, for example Linux or Windows")
set(FVM_TARGET_PROCESSOR "" CACHE STRING "Target CPU, for example aarch64 or AMD64")
set(FVM_TARGET_CC "" CACHE FILEPATH "Absolute path to target C compiler or host wrapper")
set(FVM_TARGET_CXX "" CACHE FILEPATH "Absolute target C++ compiler; required by FlashVM")
set(FVM_TARGET_SYSROOT "" CACHE PATH "Optional target sysroot; never a host include directory")
set(FVM_TARGET_AR "" CACHE FILEPATH "Optional target-aware static archive tool")
set(FVM_TARGET_RANLIB "" CACHE FILEPATH "Optional target-aware archive index tool")
set(FVM_TARGET_RC "" CACHE FILEPATH "Optional Windows target resource compiler")

if(NOT FVM_TARGET_SYSTEM OR NOT FVM_TARGET_PROCESSOR)
  message(FATAL_ERROR "Set FVM_TARGET_SYSTEM and FVM_TARGET_PROCESSOR explicitly")
endif()
if(NOT IS_ABSOLUTE "${FVM_TARGET_CC}" OR NOT EXISTS "${FVM_TARGET_CC}")
  message(FATAL_ERROR "FVM_TARGET_CC must name an existing absolute compiler path")
endif()
set(CMAKE_SYSTEM_NAME "${FVM_TARGET_SYSTEM}")
set(CMAKE_SYSTEM_PROCESSOR "${FVM_TARGET_PROCESSOR}")
set(CMAKE_C_COMPILER "${FVM_TARGET_CC}")

# A C-only project does not need C++. If a project enables C++ later, an unset
# SDK compiler must fail instead of silently discovering the host's g++.
# This intentional nonexistent path is inspected only when C++ is enabled.
if(NOT FVM_TARGET_CXX)
  set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/FVM_TARGET_CXX_REQUIRED")
endif()

foreach(pair IN ITEMS CXX AR RANLIB RC)
  if(FVM_TARGET_${pair})
    if(NOT IS_ABSOLUTE "${FVM_TARGET_${pair}}" OR NOT EXISTS "${FVM_TARGET_${pair}}")
      message(FATAL_ERROR "FVM_TARGET_${pair} must name an existing absolute tool path")
    endif()
    if(pair STREQUAL "CXX")
      set(CMAKE_CXX_COMPILER "${FVM_TARGET_${pair}}")
    elseif(pair STREQUAL "AR")
      set(CMAKE_AR "${FVM_TARGET_${pair}}")
    elseif(pair STREQUAL "RANLIB")
      set(CMAKE_RANLIB "${FVM_TARGET_${pair}}")
    else()
      set(CMAKE_RC_COMPILER "${FVM_TARGET_${pair}}")
    endif()
  endif()
endforeach()

if(FVM_TARGET_SYSROOT)
  if(NOT IS_ABSOLUTE "${FVM_TARGET_SYSROOT}" OR NOT IS_DIRECTORY "${FVM_TARGET_SYSROOT}")
    message(FATAL_ERROR "FVM_TARGET_SYSROOT must name an existing absolute SDK directory")
  endif()
  set(CMAKE_SYSROOT "${FVM_TARGET_SYSROOT}")
  set(CMAKE_FIND_ROOT_PATH "${FVM_TARGET_SYSROOT}")
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Keep executable compiler/linker probes enabled: static-only probes can hide a
# missing target runtime. Use a fresh build directory for each OS/CPU/compiler.
