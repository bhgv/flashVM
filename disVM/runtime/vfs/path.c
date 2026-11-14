/* Copyright (c) 2026 bhgv
 * EN: Bounded lexical virtual paths and immutable host-path snapshots.
 * RU: Ограниченные лексические virtual paths и независимые снимки host-путей.
 */
#include "internal.h"
#include <string.h>


/* EN/RU: See internal.h; borrowed input/output must not overlap.
 * См. internal.h; заимствованные input/output не должны перекрываться. */
int dvm_namespace_normalize(const char *cwd, const char *path, char *output)
{
    char joined[DVM_NS_PATH_LIMIT];
    size_t used = 1, start, length, position = 0;
    if (!cwd || cwd[0] != '/' || !path || !*path || strlen(path) >= sizeof(joined))
    {
        return 0;
    }
    if (*path == '/')
    {
        memcpy(joined, path, strlen(path) + 1);
    }
    else
    {
        size_t base = strlen(cwd), tail = strlen(path);
        if (base + tail + 2 > sizeof(joined))
        {
            return 0;
        }
        memcpy(joined, cwd, base);
        joined[base] = '/';
        memcpy(joined + base + 1, path, tail + 1);
    }
    output[0] = '/';
    while (joined[position])
    {
        while (joined[position] == '/')
        {
            position++;
        }
        start = position;
        while (joined[position] && joined[position] != '/')
        {
            position++;
        }
        length = position - start;
        if (!length || (length == 1 && joined[start] == '.'))
        {
            continue;
        }
        if (length == 2 && joined[start] == '.' && joined[start + 1] == '.')
        {
            while (used > 1 && output[used - 1] != '/')
            {
                used--;
            }
            if (used > 1)
            {
                used--;
            }
            continue;
        }
        if (used > 1)
        {
            output[used++] = '/';
        }
        memcpy(output + used, joined + start, length);
        used += length;
    }
    output[used] = 0;
    return 1;
}
