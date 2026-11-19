# Copyright (c) 2026 bhgv
# EN: Build the monotonic-clock example exposed by the installed runtime SDK.
# RU: Сборка примера монотонных часов, доступного в установленном runtime SDK.
# EN: Inherits root/includes; the installed example does not need a GUI context.
# RU: Использует root/includes; установленному примеру не нужен графический контекст.

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
    "${root}/ide/dis/tests/millisec_test.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/tests/millisec_test.b" VERBATIM)
add_custom_target(disvm_millisec_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis")
