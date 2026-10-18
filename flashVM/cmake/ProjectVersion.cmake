# FlashVM development version shared by the root project and configure-refresh checks.
# VERSION.txt is a configuration input even when no CMake source file changed.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" FVM_VERSION LIMIT_COUNT 1)
