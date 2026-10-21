# CMake's Windows compiler platform files replace early archive rules.
# Reapply the explicit Zig subcommands after platform initialization, before try_compile.
# This companion belongs beside zig-windows.cmake when the toolchain is copied elsewhere.
foreach(FVM_ZIG_LANGUAGE IN ITEMS C CXX)
  set(CMAKE_${FVM_ZIG_LANGUAGE}_ARCHIVE_CREATE "<CMAKE_AR> ar qc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_${FVM_ZIG_LANGUAGE}_ARCHIVE_APPEND "<CMAKE_AR> ar q <TARGET> <LINK_FLAGS> <OBJECTS>")
  set(CMAKE_${FVM_ZIG_LANGUAGE}_ARCHIVE_FINISH "<CMAKE_AR> ranlib <TARGET>")
  # GNU Make's Windows platform also builds a temporary archive for long link object lists.
  foreach(FVM_ZIG_RULE IN ITEMS LINK_EXECUTABLE CREATE_SHARED_LIBRARY CREATE_SHARED_MODULE)
    string(REPLACE "<CMAKE_AR> qc " "<CMAKE_AR> ar qc "
      CMAKE_${FVM_ZIG_LANGUAGE}_${FVM_ZIG_RULE} "${CMAKE_${FVM_ZIG_LANGUAGE}_${FVM_ZIG_RULE}}")
  endforeach()
endforeach()
