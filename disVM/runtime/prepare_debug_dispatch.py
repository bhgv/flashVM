# Copyright (c) 2026 bhgv
# EN: Add debug admission to the checked per-opcode C/JIT dispatch boundary.
# RU: Добавить debug admission на checked per-opcode C/JIT dispatch boundary.


# EN: Adapt final hosted source, preserving ordinary batching when debugging is off.
# replace checks pinned boundaries; return owned source or raise on source drift.
# RU: Адаптировать hosted source, сохраняя ordinary batching без debug.
# replace проверяет pinned boundaries; вернуть source либо ошибку дрейфа.
def adapt(code, replace):
    code = replace(
        code,
        "if(!dvm_runtime_instruction_limit && dvm_runtime_shared_integer)",
        "if(!dvm_debug_enabled && !dvm_runtime_instruction_limit && dvm_runtime_shared_integer)",
        1,
    )
    code = replace(
        code,
        "if(dvm_runtime_instruction_limit) {",
        "if(dvm_debug_enabled || dvm_runtime_instruction_limit) {",
        1,
    )
    return '#include "debugger/debugger.h"\n' + code
