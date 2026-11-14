/* Copyright (c) 2026 bhgv
 * EN: Explicit inherited debugger pipes, independent from application stdio.
 * RU: Explicit inherited debugger pipes, независимо от application stdio.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include "thread_port.h"
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


/* EN/RU: Duplicate caller's private endpoints and start supervisor before VM owners.
 * Дублировать private endpoints caller и запустить supervisor до VM owners. */
int dvm_debug_start_fds(int input, int output)
{
    int mode;
    if (input < 3 || output < 3 || input == output || dvm_debug_enabled || !dvm_debug_session_init())
    {
        return 0;
    }
    mode = fcntl(input, F_GETFL);
    if (mode < 0 || (mode & O_ACCMODE) == O_WRONLY)
    {
        return 0;
    }
    mode = fcntl(output, F_GETFL);
    if (mode < 0 || (mode & O_ACCMODE) == O_RDONLY)
    {
        return 0;
    }
    dvm_debug_service.listener = -1;
    dvm_debug_service.input = fcntl(input, F_DUPFD_CLOEXEC, 3);
    dvm_debug_service.output = fcntl(output, F_DUPFD_CLOEXEC, 3);
    dvm_debug_service.lock = fvm_port_sync_create();
    if (dvm_debug_service.input < 0 || dvm_debug_service.output < 0 || !dvm_debug_service.lock ||
        fcntl(dvm_debug_service.input, F_SETFL, O_NONBLOCK) < 0 ||
        fcntl(dvm_debug_service.output, F_SETFL, O_NONBLOCK) < 0 ||
        pthread_create(&dvm_debug_service.thread, NULL, dvm_debug_transport, NULL))
    {
        if (dvm_debug_service.input >= 0)
        {
            close(dvm_debug_service.input);
        }
        if (dvm_debug_service.output >= 0)
        {
            close(dvm_debug_service.output);
        }
        if (dvm_debug_service.lock)
        {
            fvm_port_sync_destroy(dvm_debug_service.lock);
        }
        memset(&dvm_debug_service, 0, sizeof(dvm_debug_service));
        return 0;
    }
    dvm_debug_service.started = 1;
    dvm_debug_enabled = 1;
    return 1;
}


/* EN/RU: Send one bounded native reply, dropping a slow client's output after100ms.
 * Отправить bounded native reply, отбросить output медленного клиента через100ms. */
static void reply(const char *output)
{
    size_t count = strlen(output);
    if (dvm_debug_service.output < 0)
    {
        return;
    }
    size_t sent = 0;
    unsigned int attempt;
    for (attempt = 0; attempt < 4 && sent < count; attempt++)
    {
        struct pollfd wait = {dvm_debug_service.output, POLLOUT, 0};
        ssize_t bytes;
        if (poll(&wait, 1, 25) <= 0)
        {
            continue;
        }
        bytes = write(dvm_debug_service.output, output + sent, count - sent);
        if (bytes > 0)
        {
            sent += (size_t)bytes;
        }
    }
    if (sent != count)
    {
        /* A truncated stream cannot be reused as if it had complete replies. */
        close(dvm_debug_service.output);
        dvm_debug_service.output = -1;
    }
}


/* EN: Frame native commands, bounded256 input/4096 output bytes. EOF preserves
 * retained targets and waits without spinning; runtime shutdown releases gates.
 * RU: Разделять native commands, максимум256 input/4096 output bytes. EOF сохраняет
 * retained targets и ждёт без spinning; runtime shutdown освобождает gates. */
void dvm_debug_pipe_loop(void)
{
    char command[256];
    char output[4096];
    size_t used = 0;
    int discard = 0;
    while (!__atomic_load_n(&dvm_debug_service.stopping, __ATOMIC_ACQUIRE))
    {
        char chunk[256];
        struct pollfd wait = {dvm_debug_service.input, POLLIN, 0};
        ssize_t count;
        ssize_t index;
        if (poll(&wait, 1, 25) <= 0)
        {
            continue;
        }
        count = read(dvm_debug_service.input, chunk, sizeof(chunk));
        if (count <= 0)
        {
            fvm_port_thread_sleep_us(25000);
            continue;
        }
        for (index = 0; index < count; index++)
        {
            if (chunk[index] == '\n')
            {
                command[used] = 0;
                if (!discard)
                {
                    dvm_debug_command(command, output, sizeof(output));
                    reply(output);
                }
                used = 0;
                discard = 0;
            }
            else if (used + 1 < sizeof(command) && !discard)
            {
                command[used++] = chunk[index];
            }
            else
            {
                discard = 1;
            }
        }
    }
}
