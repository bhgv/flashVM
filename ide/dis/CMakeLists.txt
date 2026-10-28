# Independent build of the portable Limbo module; native compiler is an explicit host dependency.
cmake_minimum_required(VERSION 3.10)
project(flashvm_ide_dis VERSION 0.13.0 LANGUAGES NONE)
set(LIMBO_EXECUTABLE "" CACHE FILEPATH "Explicit trusted native Limbo compiler")
if(NOT EXISTS "${LIMBO_EXECUTABLE}")
  message(FATAL_ERROR "Provide LIMBO_EXECUTABLE; this build never downloads a compiler")
endif()
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis" "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl"
  COMMAND ${CMAKE_COMMAND} "-DLIMBO=${LIMBO_EXECUTABLE}"
    "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/idecore.b" "-DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/idecore.dis" "-DSYMBOLS=${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/compile_module.cmake"
  DEPENDS idecore.b idecore.m compile_module.cmake "${LIMBO_EXECUTABLE}" VERBATIM)
add_custom_target(ide_dis ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis"
  "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl")

# Workspace ownership requires only idecore, independent of the UI codec SDK.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace.sbl"
  COMMAND "${LIMBO_EXECUTABLE}" -g -I "${CMAKE_CURRENT_SOURCE_DIR}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -o "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace.dis"
    "${CMAKE_CURRENT_SOURCE_DIR}/bridge/ideworkspace.b"
  DEPENDS bridge/ideworkspace.b bridge/ideworkspace.m idecore.m "${LIMBO_EXECUTABLE}" VERBATIM)
add_custom_target(ide_workspace ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace.dis"
  "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace.sbl")

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/workspace-example.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/workspace-example.sbl"
  COMMAND "${LIMBO_EXECUTABLE}" -g -I "${CMAKE_CURRENT_SOURCE_DIR}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -I "${CMAKE_CURRENT_SOURCE_DIR}/examples/workspace"
    -o "${CMAKE_CURRENT_BINARY_DIR}/workspace-example.dis"
    "${CMAKE_CURRENT_SOURCE_DIR}/examples/workspace/main.b"
  DEPENDS examples/workspace/main.b examples/workspace/main.m bridge/ideworkspace.m idecore.m
    "${LIMBO_EXECUTABLE}" VERBATIM)
add_custom_target(ide_workspace_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/workspace-example.dis"
  "${CMAKE_CURRENT_BINARY_DIR}/workspace-example.sbl")

include(CTest)
set(EMU_EXECUTABLE "" CACHE FILEPATH "Optional explicit reference Dis emulator for channel regression")
set(LIMBO_MODULE_DIR "" CACHE PATH "External module headers for the test bootstrap's Sys interface")
# File windows require only explicit Sys headers, independently of the optional UI SDK.
if(LIMBO_MODULE_DIR)
  if(NOT EXISTS "${LIMBO_MODULE_DIR}/sys.m")
    message(FATAL_ERROR "Provide LIMBO_MODULE_DIR containing sys.m for file windows")
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idefiles.dis"
      "${CMAKE_CURRENT_BINARY_DIR}/idefiles.sbl"
    COMMAND "${LIMBO_EXECUTABLE}" -g -I "${LIMBO_MODULE_DIR}"
      -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -o "${CMAKE_CURRENT_BINARY_DIR}/idefiles.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/bridge/idefiles.b"
    DEPENDS bridge/idefiles.b bridge/idefiles.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_file_windows ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idefiles.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/idefiles.sbl")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idebrowser.dis"
      "${CMAKE_CURRENT_BINARY_DIR}/idebrowser.sbl"
    COMMAND "${LIMBO_EXECUTABLE}" -g -I "${LIMBO_MODULE_DIR}"
      -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -o "${CMAKE_CURRENT_BINARY_DIR}/idebrowser.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/bridge/idebrowser.b"
    DEPENDS bridge/idebrowser.b bridge/idebrowser.m bridge/idefiles.m
      "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_directory_browser ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idebrowser.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/idebrowser.sbl")

endif()

if(BUILD_TESTING AND (LIMBO_MODULE_DIR OR EMU_EXECUTABLE))
  if(NOT EXISTS "${LIMBO_MODULE_DIR}/sys.m")
    message(FATAL_ERROR "Provide LIMBO_MODULE_DIR for compiling test/example modules")
  endif()
  if(EMU_EXECUTABLE AND NOT EXISTS "${EMU_EXECUTABLE}")
    message(FATAL_ERROR "Explicit EMU_EXECUTABLE does not exist")
  endif()
  # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
  if(CMAKE_VERSION VERSION_LESS "3.12")
    find_package(PythonInterp 3 REQUIRED)
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/idecore_test.b"
    DEPENDS tests/idecore_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_test_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/idecore_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/limits_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/limits_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/limits_test.b"
    DEPENDS tests/limits_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_limits_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/limits_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/coordinates_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/coordinates_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/coordinates_test.b"
    DEPENDS tests/coordinates_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_coordinates_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/coordinates_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/find_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/find_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/find_test.b"
    DEPENDS tests/find_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_find_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/find_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/navigation.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/navigation.dis" "${CMAKE_CURRENT_SOURCE_DIR}/examples/navigation/main.b"
    DEPENDS examples/navigation/main.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_navigation_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/navigation.dis")
  # Production history regression and a private reduced-budget fixture.
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/history_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/history_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/history_test.b"
    DEPENDS tests/history_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_history_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/history_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/history.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/history.dis" "${CMAKE_CURRENT_SOURCE_DIR}/examples/history/main.b"
    DEPENDS examples/history/main.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_history_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/history.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/replaceall_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/replaceall_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/replaceall_test.b"
    DEPENDS tests/replaceall_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_replaceall_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/replaceall_test.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/replaceall.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/replaceall.dis" "${CMAKE_CURRENT_SOURCE_DIR}/examples/replaceall/main.b"
    DEPENDS examples/replaceall/main.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_replaceall_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/replaceall.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/utf8span_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${LIMBO_MODULE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/utf8span_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/utf8span_test.b"
    DEPENDS tests/utf8span_test.b idecore.m "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_utf8span_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/utf8span_test.dis")
  set(history_root "${CMAKE_CURRENT_BINARY_DIR}/history-root")
  file(MAKE_DIRECTORY "${history_root}")
  configure_file(idecore.b "${history_root}/idecore.b" COPYONLY)
  configure_file(idecore.m "${history_root}/idecore.m" COPYONLY)
  file(READ "${history_root}/idecore.m" history_header)
  string(REPLACE "MaxHistoryEntries: con 64;" "MaxHistoryEntries: con 3;" history_header "${history_header}")
  string(REPLACE "MaxHistoryCharacters: con 1048576;" "MaxHistoryCharacters: con 6;" history_header "${history_header}")
  file(WRITE "${history_root}/idecore.m" "${history_header}")
  add_custom_command(OUTPUT "${history_root}/idecore.dis" "${history_root}/historylimits_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${history_root}" -o "${history_root}/idecore.dis" "${history_root}/idecore.b"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${history_root}" -I "${LIMBO_MODULE_DIR}"
      -o "${history_root}/historylimits_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/historylimits_test.b"
    DEPENDS tests/historylimits_test.b "${history_root}/idecore.b" "${history_root}/idecore.m"
      "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_historylimits_module ALL DEPENDS "${history_root}/idecore.dis" "${history_root}/historylimits_test.dis")
  set(revision_root "${CMAKE_CURRENT_BINARY_DIR}/revision-root")
  file(MAKE_DIRECTORY "${revision_root}")
  configure_file(idecore.b "${revision_root}/idecore.b" COPYONLY)
  configure_file(idecore.m "${revision_root}/idecore.m" COPYONLY)
  file(READ "${revision_root}/idecore.m" revision_header)
  string(REPLACE "MaxRevision: con 2147483647;" "MaxRevision: con 4;" revision_header "${revision_header}")
  file(WRITE "${revision_root}/idecore.m" "${revision_header}")
  add_custom_command(OUTPUT "${revision_root}/idecore.dis" "${revision_root}/revision_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${revision_root}" -o "${revision_root}/idecore.dis"
      "${revision_root}/idecore.b"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${revision_root}" -I "${LIMBO_MODULE_DIR}"
      -o "${revision_root}/revision_test.dis" "${CMAKE_CURRENT_SOURCE_DIR}/tests/revision_test.b"
    DEPENDS tests/revision_test.b "${revision_root}/idecore.b" "${revision_root}/idecore.m"
      "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_dis_revision_module ALL DEPENDS "${revision_root}/idecore.dis"
    "${revision_root}/revision_test.dis")
  # Compile the clients regardless of runtime availability. Only execution
  # tests depend on a reference emulator; product checks work without one.
  if(EMU_EXECUTABLE)
  foreach(mode 0 1)
    add_test(NAME ide_dis_utf8span_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module utf8span_test.dis)
    set_tests_properties(ide_dis_utf8span_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_replaceall_example_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module replaceall.dis)
    set_tests_properties(ide_dis_replaceall_example_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_replaceall_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module replaceall_test.dis)
    set_tests_properties(ide_dis_replaceall_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_history_example_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module history.dis)
    set_tests_properties(ide_dis_history_example_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_history_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module history_test.dis)
    add_test(NAME ide_dis_historylimits_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${history_root}" --mode ${mode} --module historylimits_test.dis)
    set_tests_properties(ide_dis_history_mode_${mode} ide_dis_historylimits_mode_${mode}
      PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_navigation_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module navigation.dis)
    set_tests_properties(ide_dis_navigation_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_find_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module find_test.dis)
    set_tests_properties(ide_dis_find_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_coordinates_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module coordinates_test.dis)
    set_tests_properties(ide_dis_coordinates_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
    add_test(NAME ide_dis_reference_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module idecore_test.dis)
    add_test(NAME ide_dis_limits_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode} --module limits_test.dis)
    add_test(NAME ide_dis_revision_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/run.py" --emu "${EMU_EXECUTABLE}"
      --build "${revision_root}" --mode ${mode} --module revision_test.dis)
  endforeach()
  set_tests_properties(ide_dis_reference_mode_0 ide_dis_reference_mode_1 ide_dis_limits_mode_0 ide_dis_limits_mode_1 ide_dis_revision_mode_0 ide_dis_revision_mode_1 PROPERTIES RUN_SERIAL TRUE TIMEOUT 40)
  endif()
endif()

if(BUILD_TESTING)
  # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
  if(CMAKE_VERSION VERSION_LESS "3.12")
    find_package(PythonInterp 3 REQUIRED)
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()
  add_test(NAME ide_dis_compiler_products COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/build_guard.py" "${CMAKE_COMMAND}"
    "${CMAKE_CURRENT_SOURCE_DIR}/compile_module.cmake")
endif()

# Install only production module/interface/debug data, never bootstrap tests or reduced-limit fixtures.
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/idecore.dis" "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl"
  "${CMAKE_CURRENT_SOURCE_DIR}/idecore.m" DESTINATION share/flashvm/ide/dis)

# Optional independent reader verifies the emitted source-map prefix, without a sibling dependency.
set(SBLMAP_EXECUTABLE "" CACHE FILEPATH "Optional explicit C99 sblmap reader")
if(BUILD_TESTING AND SBLMAP_EXECUTABLE)
  if(NOT EXISTS "${SBLMAP_EXECUTABLE}")
    message(FATAL_ERROR "SBLMAP_EXECUTABLE does not exist")
  endif()
  # FindPython3 starts at CMake 3.12; retain standalone 3.10 builds.
  if(CMAKE_VERSION VERSION_LESS "3.12")
    find_package(PythonInterp 3 REQUIRED)
    set(Python3_EXECUTABLE "${PYTHON_EXECUTABLE}")
    set(Python3_Interpreter_FOUND "${PYTHONINTERP_FOUND}")
  else()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
  endif()
  add_test(NAME ide_dis_source_map COMMAND ${Python3_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/symbols_check.py" "${SBLMAP_EXECUTABLE}"
    "${CMAKE_CURRENT_BINARY_DIR}/idecore.sbl")
endif()

# Optional cross-component test: an explicit real FIR compiler produces the span.
# Standalone IDE builds never assume that a sibling FIR build exists.
set(FIR_SOURCE_EXECUTABLE "" CACHE FILEPATH "Optional explicit FIR source compiler for diagnostic navigation")
if(BUILD_TESTING AND EMU_EXECUTABLE AND FIR_SOURCE_EXECUTABLE)
  if(NOT EXISTS "${FIR_SOURCE_EXECUTABLE}")
    message(FATAL_ERROR "FIR_SOURCE_EXECUTABLE does not exist")
  endif()
  foreach(mode 0 1)
    add_test(NAME ide_source_diagnostic_navigation_mode_${mode} COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/source_navigation.py"
      --source-compiler "${FIR_SOURCE_EXECUTABLE}" --limbo "${LIMBO_EXECUTABLE}"
      --module-dir "${LIMBO_MODULE_DIR}" --emu "${EMU_EXECUTABLE}"
      --build "${CMAKE_CURRENT_BINARY_DIR}" --mode ${mode})
    set_tests_properties(ide_source_diagnostic_navigation_mode_${mode}
      PROPERTIES RUN_SERIAL TRUE TIMEOUT 80)
  endforeach()
endif()

# Optional matching reference-runtime test for the independent workspace owner.
if(BUILD_TESTING AND LIMBO_MODULE_DIR)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace_test.dis"
    COMMAND "${LIMBO_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}"
      -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -I "${CMAKE_CURRENT_SOURCE_DIR}/examples/workspace"
      -I "${LIMBO_MODULE_DIR}" -o "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace_test.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/ideworkspace_test.b"
    DEPENDS tests/ideworkspace_test.b examples/workspace/main.m bridge/ideworkspace.m idecore.m
      "${LIMBO_MODULE_DIR}/sys.m" "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(ide_workspace_test ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ideworkspace_test.dis")
  if(EMU_EXECUTABLE)
    foreach(mode 0 1)
      add_test(NAME ideworkspace_test_mode_${mode} COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_bridge.py" --emu "${EMU_EXECUTABLE}"
        --build "${CMAKE_CURRENT_BINARY_DIR}" --module ideworkspace_test --mode ${mode})
      set_tests_properties(ideworkspace_test_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 30)
    endforeach()
  endif()
endif()

# Optional explicit shared transport source SDK; standalone document builds remain independent.
set(UIBRIDGE_SOURCE_ROOT "" CACHE PATH "DisVM source root containing module/uibridge.m and limbo/uibridge.b")
if(UIBRIDGE_SOURCE_ROOT)
  if(NOT EXISTS "${UIBRIDGE_SOURCE_ROOT}/module/uibridge.m"
      OR NOT EXISTS "${UIBRIDGE_SOURCE_ROOT}/limbo/uibridge.b")
    message(FATAL_ERROR "UIBRIDGE_SOURCE_ROOT must contain the shared Limbo codec sources")
  endif()

  # Compile a real Dis module plus debug symbols using only the explicit source-built compiler.
  # module/source name the product and source; additional headers are declared dependencies.
  function(ide_bridge_module module source)
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis"
        "${CMAKE_CURRENT_BINARY_DIR}/${module}.sbl"
      COMMAND "${LIMBO_EXECUTABLE}" -g -I "${UIBRIDGE_SOURCE_ROOT}/module"
        -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge"
        -o "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis" "${source}"
      DEPENDS "${source}" "${LIMBO_EXECUTABLE}" idecore.m bridge/idebridge.m
        bridge/ideinput.m bridge/idecomplete.m bridge/idebuiltins.m bridge/idevalue.m "${UIBRIDGE_SOURCE_ROOT}/module/uibridge.m" VERBATIM)
    add_custom_target(ide_bridge_${module} ALL DEPENDS
      "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${module}.sbl")
  endfunction()

  ide_bridge_module(uibridge "${UIBRIDGE_SOURCE_ROOT}/limbo/uibridge.b")
  foreach(module ideinput idebridge idecomplete idebuiltins ideviewport idefileops idesearch)
    ide_bridge_module(${module} "${CMAKE_CURRENT_SOURCE_DIR}/bridge/${module}.b")
  endforeach()

  if(BUILD_TESTING AND LIMBO_MODULE_DIR)
    foreach(module idebridge_test idecomplete_test idebuiltins_test)
      add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis"
        COMMAND "${LIMBO_EXECUTABLE}" -g -I "${UIBRIDGE_SOURCE_ROOT}/module"
          -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge"
          -I "${LIMBO_MODULE_DIR}" -o "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis"
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/${module}.b"
        DEPENDS "tests/${module}.b" bridge/idebridge.m bridge/ideinput.m bridge/idecomplete.m bridge/idebuiltins.m bridge/idevalue.m
          idecore.m "${UIBRIDGE_SOURCE_ROOT}/module/uibridge.m" "${LIMBO_EXECUTABLE}" VERBATIM)
      add_custom_target(ide_bridge_${module} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${module}.dis")
      if(EMU_EXECUTABLE)
        foreach(mode 0 1)
          add_test(NAME ${module}_mode_${mode} COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_bridge.py" --emu "${EMU_EXECUTABLE}"
            --build "${CMAKE_CURRENT_BINARY_DIR}" --module "${module}" --mode ${mode})
          set_tests_properties(${module}_mode_${mode} PROPERTIES RUN_SERIAL TRUE TIMEOUT 30)
        endforeach()
      endif()
    endforeach()
  endif()
endif()

# Optional developer checks use explicit tools; neither Node nor Python runs the deployed IDE.
set(IDE_AS_SOURCE_ROOT "" CACHE PATH "Explicit AS view/worker source directory")
set(AS2_PARSE_EXECUTABLE "" CACHE FILEPATH "Optional own Coco/R AS2 parser")
set(NODE_EXECUTABLE "" CACHE FILEPATH "Optional Node algorithm-test host")
if(BUILD_TESTING AND IDE_AS_SOURCE_ROOT)
  if(AS2_PARSE_EXECUTABLE)
    foreach(source syntax-worker.as presentation.as)
      add_test(NAME ide_as_${source} COMMAND "${AS2_PARSE_EXECUTABLE}"
        "${IDE_AS_SOURCE_ROOT}/${source}")
    endforeach()
  endif()
  if(NODE_EXECUTABLE)
    add_test(NAME ide_as_algorithms COMMAND "${NODE_EXECUTABLE}" "${IDE_AS_SOURCE_ROOT}/tests.js"
      "${UIBRIDGE_SOURCE_ROOT}/examples/ui-channel/bridge.as")
    set_tests_properties(ide_as_algorithms PROPERTIES TIMEOUT 20)
  endif()
endif()


# Projection services are pure modules and require no native UI SDK.
foreach(service idevalue ideprojection ideinspect idedocprojection)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${service}.dis"
      "${CMAKE_CURRENT_BINARY_DIR}/${service}.sbl"
    COMMAND "${LIMBO_EXECUTABLE}" -g -I "${CMAKE_CURRENT_SOURCE_DIR}/bridge" -I "${CMAKE_CURRENT_SOURCE_DIR}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/${service}.dis"
      "${CMAKE_CURRENT_SOURCE_DIR}/bridge/${service}.b"
    DEPENDS "bridge/${service}.b" bridge/ideprojection.m bridge/ideinspect.m bridge/idevalue.m bridge/idedocprojection.m idecore.m
      "${LIMBO_EXECUTABLE}" VERBATIM)
  add_custom_target(${service} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${service}.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/${service}.sbl")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${service}.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/${service}.sbl" DESTINATION lib/dis)
endforeach()
install(FILES bridge/ideprojection.m bridge/ideinspect.m bridge/idevalue.m bridge/idedocprojection.m idecore.m DESTINATION include/limbo)
