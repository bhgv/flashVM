/* Copyright (c) 2026 bhgv
 * EN: Compose private runtime source modules in their dependency order. They
 * retain one translation unit: no public ABI or global execution lock is added.
 * RU: Подключить закрытые модули runtime в порядке зависимостей. Они сохраняют
 * единый translation unit без нового публичного ABI или общего замка исполнения.
 */
#include "host_runtime/host_state.inc"
#include "host_runtime/diagnostics_and_files.inc"
#include "host_runtime/cancellations_and_stdio.inc"
#include "host_runtime/socket_stdio.inc"
#include "host_runtime/input_and_arena.inc"
#include "host_runtime/errors_and_spawn.inc"
#include "host_runtime/channel_claims.inc"
#include "host_runtime/process_control.inc"
#include "host_runtime/channel_retirement.inc"
#include "host_runtime/modules_and_boot.inc"
#include "host_runtime/owner_requests.inc"
#include "host_runtime/debug_location.inc"
#include "host_runtime/dispatch.inc"
#include "host_runtime/execution.inc"
#include "host_runtime/options.inc"
#include "host_runtime/system_entries.inc"
