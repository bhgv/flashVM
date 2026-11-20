/* Copyright (c) 2026 bhgv
 * EN: Launch payload for a client process inside a granted loaded module.
 * RU: Данные запуска процесса клиента внутри разрешённого загруженного модуля.
 */
#ifndef STYX_LAUNCH_PROGRAM_RECORD_H
#define STYX_LAUNCH_PROGRAM_RECORD_H
#include "launch/ticket.h"

#define STYX_LAUNCH_PROGRAM_HEADER 13U /* Version byte and three network-order u32 fields. */

int styxlaunchprogramencode(void *header, const DVM_LaunchProgram *program);
int styxlaunchprogramdecode(const void *bytes, size_t size, DVM_LaunchProgram *program);

#endif
