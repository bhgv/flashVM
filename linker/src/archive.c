/* SWA1 whole-member libraries: bounded records, no filesystem/search policy. */
#include "internal.h"
#include <stdlib.h>
#include <string.h>


int swl_add_archive(SwlContext *c, const void *data, size_t size, char *e, size_t cap)
{
    const unsigned char *p = (const unsigned char *)data;
    size_t pos = 12, i, old_count, old_bytes;
    uint32_t count, n;
    if (!c || !p || size < 12 || size > SWL_LIMIT || memcmp(p, "SWA1", 4) || swl_u32(p + 4) != 1)
    {
        return swl_fail(e, cap, "invalid SWA1 header/version");
    }
    count = swl_u32(p + 8);
    old_count = c->count;
    old_bytes = c->bytes;
    if (!count || count > SWL_ITEMS)
    {
        return swl_fail(e, cap, "invalid archive member count");
    }
    for (i = 0; i < count; ++i)
    {
        if (size - pos < 4)
        {
            goto malformed;
        }
        n = swl_u32(p + pos);
        pos += 4;
        if (n > size - pos)
        {
            goto malformed;
        }
        if (!swl_add_object(c, p + pos, n, e, cap))
        {
            goto rollback;
        }
        pos += n;
    }
    if (pos != size)
    {
        goto malformed;
    }
    return 1;
malformed:
    swl_fail(e, cap, "invalid SWA1 record lengths/trailing bytes");
rollback:
    while (c->count > old_count)
    {
        free(c->objects[--c->count].data);
    }
    c->bytes = old_bytes;
    return 0;
}


int swl_archive(const SwlObject *objects, size_t count, SwlWriter writer, void *user, char *e, size_t cap)
{
    SwlContext *check;
    Buffer b = {0};
    size_t i;
    int ok = 0;
    if (!objects || !count || count > SWL_ITEMS || !writer)
    {
        return swl_fail(e, cap, "invalid archive arguments");
    }
    check = swl_create(NULL);
    if (!check)
    {
        return swl_fail(e, cap, "out of memory");
    }
    swl_bytes(&b, "SWA1", 4);
    swl_dword(&b, 1);
    swl_dword(&b, (uint32_t)count);
    for (i = 0; i < count; ++i)
    {
        if (!swl_add_object(check, objects[i].data, objects[i].size, e, cap))
        {
            goto done;
        }
        swl_dword(&b, (uint32_t)objects[i].size);
        swl_bytes(&b, objects[i].data, objects[i].size);
    }
    if (b.failed)
    {
        swl_fail(e, cap, "archive size/allocation limit exceeded");
        goto done;
    }
    ok = writer(user, b.data, b.size);
    if (!ok)
    {
        swl_fail(e, cap, "output callback failed");
    }
done:
    free(b.data);
    swl_destroy(check);
    return ok;
}
