set(FVM_NANOSVG_SOURCES nanosvg.cpp)
add_library(fvm_nanosvg STATIC ${FVM_NANOSVG_SOURCES})
target_include_directories(fvm_nanosvg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream)
if(NOT WIN32)
  target_link_libraries(fvm_nanosvg PUBLIC m)
endif()

# Upstream sources use C99 integer/types; keep this private to the vendor target.
set_target_properties(fvm_nanosvg PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS OFF)
