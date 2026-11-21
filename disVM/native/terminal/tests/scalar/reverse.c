/* Copyright (c) 2026 bhgv
 * EN: Exercise the call-scoped virtual PID file over a real reverse Styx session.
 * RU: Проверить виртуальный .pid вызова через встречную сессию Styx.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "client.h"
#include "mount.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fd_reads; /* Successful shared-cursor checks. */


/* EN: Mount the provider side of the reverse session, consuming the transport.
 * RU: Смонтировать встречную сессию на стороне провайдера, приняв транспорт. */
DVM_Provider *scalar_reverse_mount(RemotePhy *transport)
{
    StyxClient *client = styxclientmountphy(transport, 512, "provider", "", 3000, 4);
    return client ? styxmountprovider(client, FVM_ACCESS_READ) : NULL;
}


/* EN: Compare an exact virtual file to its expected fixture value over Styx.
 * RU: Сверить виртуальный файл с ожидаемым значением через Styx. */
static int check_file(DVM_Provider *self, const char *relative, const char *expected)
{
    char bytes[12];
    DVM_VfsHandle file = {0};
    if (dvm_provider_open(self, relative, "r", &file) != FVM_OK)
    {
        return 0;
    }
    size_t length = strlen(expected);
    DVM_IoResult result = dvm_vfs_read_chunk(&file, bytes, length);
    dvm_vfs_close(&file);
    return result.status == FVM_OK && result.count == length && !memcmp(expected, bytes, length);
}


/* EN: Check the selected .self and the inherited descriptor via its mounted provider.
 * RU: Проверить выбранный .self и унаследованный FD через смонтированный провайдер. */
int scalar_reverse_check(DVM_Provider *self, int64_t argument)
{
    uint32_t pid = (uint32_t)(argument < 0 ? -argument : argument);
    char expected[12];
    snprintf(expected, sizeof(expected), "%" PRIu32 "\n", pid);
    if (argument < 0)
    {
        return fd_reads == 2 && check_file(self, ".pid", expected);
    }
    int ok = fd_reads < 2 && check_file(self, ".pid", expected) &&
        check_file(self, ".fd/3", fd_reads ? "next" : "left") &&
        check_file(self, ".vfs/env/CLIENT", "/.vfs") && check_file(self, ".var/counter", "/.var");
    fd_reads += ok;
    return ok;
}
