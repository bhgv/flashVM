/* Copyright (c) 2026 bhgv
 * EN: Validate channel grants and open their live nodes without ambient namespace resolution.
 * RU: Проверка channel grants и открытие live nodes без поиска в окружающем namespace.
 */
#include "terminal.h"
#include "../cancel.h"
#include <string.h>


/* EN/RU: Check a borrowed NUL-terminated path within capacity; dot denotes the granted root.
 * Reject absolute, empty and dot/dot-dot components rather than rewriting authority boundaries.
 * Проверить borrowed path в capacity; dot — root; absolute/empty/dot-components запрещены. */
static int relative(const char *path, size_t capacity)
{
    if (!path || !path[0] || path[0] == '/')
    {
        return 0;
    }
    size_t length = 0;
    while (length < capacity && path[length])
    {
        length++;
    }
    if (length == capacity)
    {
        return 0;
    }
    if (!strcmp(path, "."))
    {
        return 1;
    }
    const char *part = path;
    for (const char *next = path; ; next++)
    {
        if (*next == '/' || !*next)
        {
            size_t size = (size_t)(next-part);
            if (!size || (size == 1 && part[0] == '.') ||
                (size == 2 && part[0] == '.' && part[1] == '.'))
            {
                return 0;
            }
            if (!*next)
            {
                return 1;
            }
            part = next+1;
        }
    }
}


/* EN/RU: See terminal.h; immutable capability checks never call provider code.
 * См. terminal.h; immutable checks не вызывают provider code. */
int dvm_terminal_channels_valid(const DVM_TerminalChannels *channels)
{
    return channels && channels->provider &&
        (dvm_provider_native_flags(channels->provider) & DVM_PROVIDER_CANCEL_PATH) &&
        dvm_provider_bindable_at(channels->provider, ".") &&
        relative(channels->receive, sizeof(channels->receive)) &&
        relative(channels->send, sizeof(channels->send));
}


/* EN/RU: See terminal.h; preserve backend status and ownership, with no alternative provider.
 * См. terminal.h; сохранить backend status/ownership, без поиска другого provider. */
FVM_Result dvm_terminal_channel_open(const DVM_TerminalChannels *channels, const char *path,
    const char *mode, DVM_VfsHandle *out)
{
    if (!dvm_terminal_channels_valid(channels) || !relative(path, DVM_NS_PATH_LIMIT) ||
        !mode || (strcmp(mode, "r") && strcmp(mode, "w") && strcmp(mode, "r+") && strcmp(mode, "rw")) ||
        !out || out->ops || out->state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    const char *native_mode = !strcmp(mode, "r") ? "r" : !strcmp(mode, "w") ? "wo" : "rw";
    return dvm_provider_open(channels->provider, path, native_mode, out);
}
