/* Copyright (c) 2026 bhgv
 * EN: Validate channel names shared by Styx clients and providers.
 * RU: Проверять имена каналов в клиентах и провайдерах Styx.
 */
#include "styxrpc/named_channel.h"
#include <string.h>


/* EN: Recognize one reserved VFS component without matching a longer ordinary name.
 * RU: Распознать служебный компонент VFS, сохраняя обычные имена с тем же префиксом. */
static int reserved(const char *name, size_t size)
{
    static const char *const names[] = {".", "..", ".fd", ".prog", ".pid", ".var", ".vfs",
        ".unibus", ".rpc", ".messages", STYXRPC_NAMED_SERVICE, ".interface", ".programs",
        ".sysevents", ".state", ".upload", ".exit", ".idx", ".val", ".len", ".type",
        ".ins", ".push", ".pop", ".add", ".count"};
    for (size_t i = 0; i < sizeof(names) / sizeof(*names); ++i)
    {
        if (strlen(names[i]) == size && !memcmp(names[i], name, size))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See named_channel.h. / См. named_channel.h. */
int styxrpc_named_path(const char *path)
{
    if (!path || *path != '/' || strlen(path) >= STYXRPC_VFS_PATH_MAX || !path[1])
    {
        return 0;
    }
    for (const char *part = path + 1; ; )
    {
        size_t size = strcspn(part, "/");
        if (!size || (*part == '.' && reserved(part, size)))
        {
            return 0;
        }
        if (!part[size])
        {
            return 1;
        }
        part += size + 1;
    }
}
