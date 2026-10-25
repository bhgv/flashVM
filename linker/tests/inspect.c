/* Independent raw SWO fixtures exercise publication and cancellation boundaries. */
#include "swl.h"
#include <stdlib.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
            return __LINE__;                                                                                           \
    } while (0)


typedef struct
{
    unsigned calls;
    int cancel;
} Visit;


typedef struct
{
    unsigned char *data;
    size_t size;
} Bytes;


static int capture(void *user, const unsigned char *data, size_t size)
{
    Bytes *out = (Bytes *)user;
    out->data = (unsigned char *)malloc(size);
    if (!out->data)
    {
        return 0;
    }
    memcpy(out->data, data, size);
    out->size = size;
    return 1;
}


static int visit(void *user, const SwlModuleInfo *info)
{
    Visit *v = (Visit *)user;
    if (info->code_size != 1 || info->requirements != v->calls || strcmp(info->name, v->calls ? "b" : "a"))
    {
        return 0;
    }
    ++v->calls;
    return !v->cancel;
}


static void make_object(unsigned char *data, unsigned name, unsigned flags)
{
    uint32_t hash = (UINT32_C(2166136261) ^ name) * UINT32_C(16777619);
    unsigned i;
    memset(data, 0, 26);
    memcpy(data, "SWO1", 4);
    data[4] = 1;
    data[8] = (unsigned char)flags;
    data[12] = 1;
    data[16] = 1;
    data[24] = (unsigned char)name;
    hash *= UINT32_C(16777619);
    for (i = 0; i < 4; ++i)
    {
        data[20 + i] = (unsigned char)(hash >> (8 * i));
    }
}


int main(void)
{
    unsigned char a[26], b[26];
    SwlObject objects[2];
    Bytes archive = {0};
    Visit v = {0};
    char error[128];
    size_t n;
    make_object(a, 'a', 0);
    make_object(b, 'b', 1);
    objects[0].data = a;
    objects[0].size = sizeof(a);
    objects[1].data = b;
    objects[1].size = sizeof(b);
    CHECK(swl_archive(objects, 2, capture, &archive, error, sizeof(error)));
    CHECK(swl_inspect(archive.data, archive.size, visit, &v, error, sizeof(error)));
    CHECK(v.calls == 2);
    for (n = 0; n < archive.size; ++n)
    {
        v.calls = 0;
        CHECK(!swl_inspect(archive.data, n, visit, &v, NULL, 0));
        CHECK(v.calls == 0);
    }
    archive.data[archive.size - 1] = 1;
    v.calls = 0;
    CHECK(!swl_inspect(archive.data, archive.size, visit, &v, error, sizeof(error)));
    CHECK(v.calls == 0);
    archive.data[archive.size - 1] = 0;
    v.cancel = 1;
    CHECK(!swl_inspect(archive.data, archive.size, visit, &v, error, sizeof(error)));
    CHECK(v.calls == 1 && strstr(error, "cancelled"));
    v.cancel = 0;
    v.calls = 0;
    CHECK(swl_inspect(a, sizeof(a), visit, &v, error, sizeof(error)));
    CHECK(v.calls == 1);
    CHECK(!swl_inspect(a, sizeof(a), NULL, NULL, error, sizeof(error)));
    CHECK(!swl_inspect(NULL, sizeof(a), visit, &v, error, sizeof(error)));
    free(archive.data);
    return 0;
}
