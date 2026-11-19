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

# EN/RU: Menu entry 19 runs permanent native owners, without removed pool options.
# Пункт 19 запускает закреплённых владельцев без опций удалённого пула.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/resident-owners.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/resident-owners.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
        -o "${CMAKE_CURRENT_BINARY_DIR}/resident-owners.dis"
        "${root}/disVM/examples/resident-owners/main.b"
    DEPENDS limbo-bootstrap "${root}/disVM/examples/resident-owners/main.b"
        ${module_headers} VERBATIM)
add_custom_target(disvm_resident_owners ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/resident-owners.dis")
add_dependencies(disvm disvm_resident_owners)
if(DVM_RUNTIME_EDITOR)
    add_dependencies(disvm-editor disvm_resident_owners)
endif()
