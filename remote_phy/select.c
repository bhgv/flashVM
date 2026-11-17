/* Copyright (c) 2026 bhgv
 * EN: Explicit host-supplied physical-driver selection, with no global registry or implicit fallback.
 * RU: Явный выбор physical driver из host table без global registry и implicit fallback.
 */
#include "phy.h"
#include <string.h>


/* EN/RU: See phy.h; reject ambiguous names before invoking any factory.
 * См. phy.h; ambiguous names запрещены до вызова factory. */
RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name,
    const void *parameters, size_t size)
{
    if ((!drivers && count) || !name || !*name || (!parameters && size))
    {
        return NULL;
    }
    const PhyDriver *found = NULL;
    for (size_t i = 0; i < count; i++)
    {
        if (drivers[i].name && !strcmp(drivers[i].name, name))
        {
            if (found || !drivers[i].open)
            {
                return NULL;
            }
            found = drivers+i;
        }
    }
    return found ? found->open(parameters, size) : NULL;
}
