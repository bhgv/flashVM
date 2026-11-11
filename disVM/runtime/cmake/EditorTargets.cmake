# Copyright (c) 2026 bhgv
# EN: Ordered EditorTargets registrations; include from the runtime source directory.
# RU: Регистрация EditorTargets; подключать из каталога исходников runtime.
if(DVM_RUNTIME_EDITOR)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fonts")
  configure_file("${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf" COPYONLY)
  configure_file("${root}/ide/assets/fonts/OFL.txt" "${CMAKE_CURRENT_BINARY_DIR}/fonts/OFL.txt" COPYONLY)
  enable_language(CXX)
  set(CMAKE_CXX_STANDARD 98)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(FVM_BUILD_TESTS OFF CACHE BOOL "Editor owns focused integration checks" FORCE)
  set(FVM_BUILD_EXAMPLES OFF CACHE BOOL "Build only the embedded FlashVM" FORCE)
  add_subdirectory("${DVM_FLASHVM_SOURCE_ROOT}" flashvm)
  get_target_property(editor_flashvm_definitions flashvm COMPILE_DEFINITIONS)
  list(FIND editor_flashvm_definitions RENDER_JOBS editor_render_jobs_index)
  if(editor_render_jobs_index EQUAL -1)
    message(FATAL_ERROR "The graphical editor requires RENDER_JOBS in the selected FlashVM DEFS")
  endif()
  get_target_property(runtime_sources disvm SOURCES)
  get_target_property(runtime_includes disvm INCLUDE_DIRECTORIES)
  add_executable(disvm-editor ${runtime_sources} editor_host.c editor_thread.c editor_bulk.c editor_view.c editor_raster.c editor_stamp.c editor_ink.c
    "${root}/disVM/host/dis_ui_channel.c")
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_editor_raster_test test_editor_raster.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_editor_raster_test PRIVATE flashvm)
    add_executable(disvm_frame_free_drawing_test test_frame_free_drawing.c editor_raster.c editor_stamp.c editor_ink.c)
    target_include_directories(disvm_frame_free_drawing_test PRIVATE "${DVM_FLASHVM_SOURCE_ROOT}/ports")
    target_link_libraries(disvm_frame_free_drawing_test PRIVATE flashvm m)
    add_test(NAME disvm_frame_free_drawing COMMAND disvm_frame_free_drawing_test)
    add_executable(disvm_movieclip_capture_test test_movieclip_capture.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_movieclip_capture_test PRIVATE flashvm m)
    add_test(NAME disvm_movieclip_native_capture COMMAND disvm_movieclip_capture_test)
    set_tests_properties(disvm_movieclip_native_capture PROPERTIES TIMEOUT 30)
    add_executable(disvm_retained_scene_test test_retained_scene.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_retained_scene_test PRIVATE flashvm m)
    add_test(NAME disvm_editor_retained_scene COMMAND disvm_retained_scene_test)
    add_executable(disvm_editor_incremental_test test_editor_incremental.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_editor_incremental_test PRIVATE flashvm m)
    add_test(NAME disvm_editor_dirty_parallel_raster COMMAND disvm_editor_incremental_test)
    set_tests_properties(disvm_editor_dirty_parallel_raster PROPERTIES TIMEOUT 30)
    add_test(NAME disvm_editor_detached_raster COMMAND disvm_editor_raster_test)
    set_tests_properties(disvm_editor_detached_raster PROPERTIES TIMEOUT 30)
  endif()
  add_dependencies(disvm-editor disvm_runtime_sources)
  if(DVM_RUNTIME_COMPILER)
    target_compile_definitions(disvm-editor PRIVATE DVM_RUNTIME_COMPILER=1)
    target_link_libraries(disvm-editor PRIVATE disvm_compiler_engine)
  endif()

  if(DVM_RUNTIME_NATIVE_DEMO)
    target_compile_definitions(disvm-editor PRIVATE DVM_RUNTIME_NATIVE_DEMO=1)
  endif()
  set(DVM_AS_VIEW_COMPILER "" CACHE FILEPATH "Source-built Coco/R AS2 display-profile compiler")
  if(NOT EXISTS "${DVM_AS_VIEW_COMPILER}")
    message(FATAL_ERROR "Build compiler/as2/view with Coco/R, then supply DVM_AS_VIEW_COMPILER")
  endif()
  set(DVM_FIR_COMPILER "" CACHE FILEPATH "FIR compiler CLI")
  set(DVM_FIR_OPTIMIZER "" CACHE FILEPATH "FIR peephole optimizer CLI")
  set(DVM_SWF_LINKER "" CACHE FILEPATH "SWL linker CLI")
  foreach(tool DVM_FIR_COMPILER DVM_FIR_OPTIMIZER DVM_SWF_LINKER)
    if(NOT EXISTS "${${tool}}")
      message(FATAL_ERROR "Supply ${tool} for AS -> FIR -> SWO -> SWF")
    endif()
  endforeach()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_custom_command(OUTPUT "${view}/ide_view_swf.h"
    BYPRODUCTS "${view}/ide-view.fir" "${view}/ide-view.opt.fir" "${view}/ide-view.swo" "${view}/ide-view.swf"
    COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/editor-view.as" "${view}/ide-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" check "${view}/ide-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_OPTIMIZER}" "${view}/ide-view.fir" "${view}/ide-view.opt.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" object "${view}/ide-view.opt.fir" "${view}/ide-view.swo"
    COMMAND "${DVM_SWF_LINKER}" link "${view}/ide-view.swf" "${view}/ide-view.swo"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_view.py"
      "${view}/ide-view.swf" "${view}/ide_view_swf.h"
    DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}"
      "${DVM_SWF_LINKER}" "${root}/ide/as/editor-view.as" embed_view.py VERBATIM)
  add_custom_command(OUTPUT "${view}/desktop_view_swf.h"
    BYPRODUCTS "${view}/desktop-view.fir" "${view}/desktop-view.opt.fir" "${view}/desktop-view.swo" "${view}/desktop-view.swf"
    COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/desktop-view.as" "${view}/desktop-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" check "${view}/desktop-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_OPTIMIZER}" "${view}/desktop-view.fir" "${view}/desktop-view.opt.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" object "${view}/desktop-view.opt.fir" "${view}/desktop-view.swo"
    COMMAND "${DVM_SWF_LINKER}" link "${view}/desktop-view.swf" "${view}/desktop-view.swo"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_view.py"
      "${view}/desktop-view.swf" "${view}/desktop_view_swf.h"
    DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}"
      "${DVM_SWF_LINKER}" "${root}/ide/as/desktop-view.as" embed_view.py VERBATIM)
  # Each chrome view is a separate Coco/R AS -> FIR -> optimizer -> SWL product.
  set(chrome_headers)
  foreach(control desktop-background rail-left rail-right window-card miniature-card window-header tab-button close-button tool-scroll tool-palette tool-pointer input-field top-bar vector-control atlas-card atlas-frame atlas-miniature)
    string(REPLACE "-" "_" symbol "chrome_${control}")
    set(header "${view}/${symbol}.h")
    add_custom_command(OUTPUT "${header}"
      COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/chrome/${control}.as" "${view}/${control}.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" check "${view}/${control}.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_OPTIMIZER}" "${view}/${control}.fir" "${view}/${control}.opt.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" object "${view}/${control}.opt.fir" "${view}/${control}.swo"
      COMMAND "${DVM_SWF_LINKER}" link "${view}/${control}.swf" "${view}/${control}.swo"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_chrome.py" "${view}/${control}.swf" "${header}" "${symbol}"
      DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}" "${DVM_SWF_LINKER}"
        "${root}/ide/as/chrome/${control}.as" embed_chrome.py VERBATIM)
    list(APPEND chrome_headers "${header}")
  endforeach()
  add_custom_target(disvm_chrome_headers DEPENDS ${chrome_headers})
  if(DVM_RUNTIME_TESTS)
    add_custom_command(OUTPUT "${view}/native-module.swf"
      COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/flashVM/examples/embedding/native_module/main.as" "${view}/native-module.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" object "${view}/native-module.fir" "${view}/native-module.swo"
      COMMAND "${DVM_SWF_LINKER}" link "${view}/native-module.swf" "${view}/native-module.swo"
      DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}" "${root}/flashVM/examples/embedding/native_module/main.as" VERBATIM)
    add_custom_target(disvm_flashvm_native_module_swf DEPENDS "${view}/native-module.swf")
    add_executable(disvm_flashvm_native_module_example
      "${root}/flashVM/examples/embedding/native_module/main.c" "${root}/flashVM/examples/embedding/native_module/module.c")
    set_property(TARGET disvm_flashvm_native_module_example PROPERTY C_STANDARD 90)
    target_link_libraries(disvm_flashvm_native_module_example PRIVATE flashvm m)
    add_dependencies(disvm_flashvm_native_module_example disvm_flashvm_native_module_swf)
    add_test(NAME disvm_flashvm_native_module_example COMMAND disvm_flashvm_native_module_example "${view}/native-module.swf")
  endif()
  # One producer owns both generated headers across parallel Make target graphs.
  add_custom_target(disvm_view_headers DEPENDS
    "${view}/ide_view_swf.h" "${view}/desktop_view_swf.h")
  add_dependencies(disvm-editor disvm_view_headers)
  target_sources(disvm-editor PRIVATE desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c)
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_desktop_colors_test test_desktop_colors.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_desktop_colors_test disvm_view_headers)
    target_include_directories(disvm_desktop_colors_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_desktop_colors_test PRIVATE flashvm m)
    add_test(NAME disvm_desktop_color_protocol COMMAND disvm_desktop_colors_test)
    add_executable(disvm_mdi_capture_test test_mdi_capture.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_mdi_capture_test disvm_view_headers)
    target_include_directories(disvm_mdi_capture_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_mdi_capture_test PRIVATE flashvm m)
    if(DVM_EDITOR_TEST_FONT)
      add_test(NAME disvm_mdi_reflow_capture COMMAND disvm_mdi_capture_test "${DVM_EDITOR_TEST_FONT}")
    endif()
    add_executable(disvm_desktop_vector_test test_desktop_vector.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_desktop_vector_test disvm_view_headers)
    target_include_directories(disvm_desktop_vector_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_desktop_vector_test PRIVATE flashvm m)
    add_test(NAME disvm_desktop_vector_protocol COMMAND disvm_desktop_vector_test)
    add_executable(disvm_as_view_test test_as_view.c)
    add_dependencies(disvm_as_view_test disvm_view_headers)
    target_include_directories(disvm_as_view_test PRIVATE "${view}")
    target_link_libraries(disvm_as_view_test PRIVATE flashvm)
    add_test(NAME disvm_as_view_execution COMMAND disvm_as_view_test)
  endif()
  target_include_directories(disvm-editor PRIVATE ${runtime_includes} "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  target_compile_definitions(disvm-editor PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_GC_FIELDS=1 DVM_RUNTIME_EDITOR=1 DVM_RUNTIME_VERSION="${PROJECT_VERSION}" DVM_RUNTIME_BACKEND="${runtime_backend}")
  target_compile_options(disvm-editor PRIVATE -fno-strict-aliasing)
  target_link_libraries(disvm-editor PRIVATE fvm_owned_table disvm_flashvm_host limbo_bootstrap_lib9
    limbo_bootstrap_support limbo_bootstrap_lib9 flashvm m)
  if(DVM_EDITOR_SDL)
    find_package(SDL2 2.0.9 CONFIG REQUIRED)
    target_sources(disvm-editor PRIVATE editor_desktop_input.c editor_motion_batch.c)
    if(DVM_RUNTIME_TESTS)
      add_executable(disvm_paint_input_test test_paint_input.c editor_desktop_input.c)
      target_link_libraries(disvm_paint_input_test PRIVATE SDL2::SDL2)
      add_test(NAME disvm_paint_input_queue COMMAND disvm_paint_input_test)
    endif()
    target_compile_definitions(disvm-editor PRIVATE DVM_EDITOR_SDL=1)
    target_sources(disvm-editor PRIVATE editor_window_events.c editor_text_events.c)
    if(DVM_RUNTIME_TESTS)
      add_executable(disvm_text_events_test test_text_events.c editor_text_events.c)
      target_include_directories(disvm_text_events_test PRIVATE "${root}/disVM/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
      target_link_libraries(disvm_text_events_test PRIVATE SDL2::SDL2 flashvm)
      add_test(NAME disvm_sdl_text_bursts COMMAND disvm_text_events_test)
      add_executable(disvm_window_events_test test_window_events.c editor_window_events.c)
      target_include_directories(disvm_window_events_test PRIVATE "${root}/disVM/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
      target_link_libraries(disvm_window_events_test PRIVATE SDL2::SDL2)
      if(UNIX AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_executable(disvm_desktop_touch_test test_desktop_touch.c editor_desktop_input.c)
        add_dependencies(disvm_desktop_touch_test disvm_view_headers)
        get_target_property(editor_includes disvm-editor INCLUDE_DIRECTORIES)
        target_include_directories(disvm_desktop_touch_test PRIVATE ${editor_includes} "${DVM_FLASHVM_SOURCE_ROOT}/include")
        target_compile_definitions(disvm_desktop_touch_test PRIVATE DVM_EDITOR_SDL=1)
        target_compile_options(disvm_desktop_touch_test PRIVATE -ffunction-sections -fdata-sections)
        target_link_options(disvm_desktop_touch_test PRIVATE -Wl,--gc-sections)
        target_link_libraries(disvm_desktop_touch_test PRIVATE SDL2::SDL2 m)
        add_test(NAME disvm_desktop_native_contacts COMMAND disvm_desktop_touch_test)
      endif()
      add_test(NAME disvm_sdl_window_lifecycle COMMAND disvm_window_events_test)
      set_tests_properties(disvm_sdl_window_lifecycle PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endif()
    target_link_libraries(disvm-editor PRIVATE SDL2::SDL2)
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
      -o "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
      "${root}/ide/dis/examples/graphical-editor/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/graphical-editor/main.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_graphical_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis")
  # Ship an explicit IDE application entry; the channel controller remains reusable.
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${CMAKE_CURRENT_BINARY_DIR}/ide.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -I "${root}/ide/dis/app"
      -o "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${root}/ide/dis/app/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/app/main.b" "${root}/ide/dis/app/ide.m"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" ${module_headers} VERBATIM)
  add_custom_target(disvm_ide_application ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ide.dis")

endif()
