/* Copyright (c) 2026 bhgv
 * EN: Owner-side lookup for host-authorized launch image slots.
 * RU: Поиск разрешённых хостом образов по номеру на стороне владельца.
 */
#ifndef DVM_TERMINAL_LAUNCH_PROGRAMS_H
#define DVM_TERMINAL_LAUNCH_PROGRAMS_H

#include "unibus_server.h"

typedef struct DVM_TerminalPrograms
{
    size_t count; /* Parsed entries; each image becomes owned at publication. */
    StyxUnibusProgram entries[STYX_LAUNCH_EXTRA_LIMIT]; /* Slot n + 1 refers to entries[n]. */
    const char *paths[STYX_LAUNCH_EXTRA_LIMIT]; /* Borrowed host-only executable paths. */
    DVM_Provider *catalog; /* Owned immutable list advertised over Styx. */
} DVM_TerminalPrograms;

/* EN: Parse repeated --execute-as NAME.dis IMAGE.dis before the separator.
 * RU: Разобрать повторяемые --execute-as NAME.dis IMAGE.dis перед разделителем. */
int dvm_terminal_parse_programs(DVM_TerminalPrograms *programs, int argc, char **argv, int *at);

/* EN: Create exactly the published names and immutable images; caller cleans up on failure.
 * RU: Создать заданные имена и неизменяемые образы; при ошибке очищает вызывающий код. */
int dvm_terminal_publish_programs(DVM_Namespace *host, DVM_TerminalPrograms *programs);

/* EN: Release all published native views after the server joins.
 * RU: Освободить опубликованные представления после join сервера. */
void dvm_terminal_release_programs(DVM_TerminalPrograms *programs);

/* EN: Borrow one session's launch authority for the duration of the runtime.
 * RU: Заимствовать полномочия запуска сессии на время работы VM. */
void dvm_terminal_configure_shared_launch(StyxUnibusServer *server, DVM_Provider *root,
    const char *image, DVM_Provider *provider, const DVM_TerminalPrograms *programs);

#endif
