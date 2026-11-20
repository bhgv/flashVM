/* Copyright (c) 2026 bhgv
 * EN: Borrowed one-shot terminal launch configuration.
 * RU: Заимствованная конфигурация одного терминального запуска.
 */
#ifndef DVM_TERMINAL_LAUNCH_CONFIG_H
#define DVM_TERMINAL_LAUNCH_CONFIG_H
#include "../../native/terminal/order/files/launch.h"
#include "../../native/launch/request.h"

#define DVM_TERMINAL_GRANTS_MAX 32U /* Maximum explicit file grants per invocation. */

typedef struct TerminalLaunchConfig
{
    const char *fd;           /* Delegated socket argument, validated before transport. */
    const char *label;        /* Host-selected launch context name. */
    const char *code;         /* Peer file exposed at /app.dis. */
    const char *dependency;   /* Peer file exposed at /dep.dis, or dash. */
    const char *request;      /* Peer file carrying the launch declaration and request. */
    StyxTerminalPaths paths;  /* Peer ordered Unibus receive/send endpoints. */
    unsigned long tag;        /* Nonzero negotiated launch tag. */
    unsigned long lane;       /* Nonzero ordered TX tag. */
    unsigned timeout;         /* Native connection/admission budget in milliseconds. */
    StyxLaunchFile files[DVM_TERMINAL_GRANTS_MAX]; /* Exact guest file grants. */
    unsigned long tags[DVM_TERMINAL_GRANTS_MAX];   /* Corresponding ordered tags. */
    size_t count;             /* Active entries in files and tags. */
    StyxLaunchFd fds[STYX_LAUNCH_FDS_MAX]; /* Exact guest aliases for terminal files. */
    size_t fd_count;          /* Active entries in fds. */
    unsigned char inherited_fds[DVM_LAUNCH_FD_COUNT]; /* Modes for numbered .fd nodes. */
    int separator;            /* Index of --, forwarded as the VM program name. */
} TerminalLaunchConfig;


/* EN/RU: Parse borrowed argv without I/O; config is caller owned, returns 1/0.
 * Разобрать заимствованные argv без I/O; config принадлежит вызывающему, 1/0. */
int dvm_terminal_launch_config(int argc, char **argv, TerminalLaunchConfig *config);
#endif
