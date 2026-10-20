/* SWO1 reader and SWF8 container writer. All decoding is explicitly bounded. */
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int swl_fail(char *error, size_t capacity, const char *message)
{
    if (error && capacity)
        snprintf(error, capacity, "%s", message);
    return 0;
}
uint32_t swl_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
void swl_bytes(Buffer *b, const void *data, size_t n)
{
    size_t capacity;
    unsigned char *next;
    if (b->failed)
        return;
    if (n > SWL_LIMIT - b->size)
    {
        b->failed = 1;
        return;
    }
    if (b->size + n > b->capacity)
    {
        capacity = b->capacity ? b->capacity : 1024;
        while (capacity < b->size + n)
            capacity *= 2;
        next = (unsigned char *)realloc(b->data, capacity);
        if (!next)
        {
            b->failed = 1;
            return;
        }
        b->data = next;
        b->capacity = capacity;
    }
    if (n)
        memcpy(b->data + b->size, data, n);
    b->size += n;
}
void swl_byte(Buffer *b, unsigned x)
{
    unsigned char v = (unsigned char)x;
    swl_bytes(b, &v, 1);
}
void swl_word(Buffer *b, unsigned x)
{
    swl_byte(b, x);
    swl_byte(b, x >> 8);
}
void swl_dword(Buffer *b, uint32_t x)
{
    swl_word(b, x);
    swl_word(b, x >> 16);
}
void swl_tag(Buffer *b, unsigned tag, const void *data, size_t n)
{
    swl_word(b, (tag << 6) | (n < 63 ? (unsigned)n : 63));
    if (n >= 63)
        swl_dword(b, (uint32_t)n);
    swl_bytes(b, data, n);
}
/* Validate each Push element, including terminators and fixed-width payloads. */
static int push(const unsigned char *p, size_t n)
{
    size_t i = 0, width;
    while (i < n)
    {
        unsigned type = p[i++];
        if (type == 0)
        {
            while (i < n && p[i])
                ++i;
            if (i == n)
                return 0;
            ++i;
        }
        else
        {
            width = type == 5 ? 1 : type == 6 ? 8 : type == 7 ? 4 : 0;
            if (!width || width > n - i)
                return 0;
            i += width;
        }
    }
    return 1;
}
/* Branches must land on instruction starts, not inside immediate data. */
static int actions(const unsigned char *p, size_t n)
{
    unsigned char *starts;
    size_t i = 0, length, end;
    int ok = 0, ended = 0;
    if (!n || n > 2u * 1024u * 1024u || p[n - 1])
        return 0;
    starts = (unsigned char *)calloc(n, 1);
    if (!starts)
        return 0;
    while (i < n)
    {
        unsigned op = p[i];
        starts[i++] = 1;
        if (!op)
        {
            if (i != n)
                goto done;
            ended = 1;
            break;
        }
        if (op >= 128)
        {
            if (n - i < 2)
                goto done;
            length = (size_t)p[i] | (size_t)p[i + 1] << 8;
            i += 2;
            if (length > n - i)
                goto done;
            if (op == 0x96)
            {
                if (!push(p + i, length))
                    goto done;
            }
            else if ((op != 0x99 && op != 0x9d) || length != 2)
                goto done;
            i += length;
        }
        else
        {
            switch (op)
            {
            case 0x1c:
            case 0x1d:
            case 0x0c:
            case 0x0d:
            case 0x60:
            case 0x64:
            case 0x0a:
            case 0x63:
            case 0x61:
            case 0x0b:
            case 0x48:
            case 0x49:
            case 0x47:
            case 0x4a:
            case 0x52:
            case 0x17:
            case 0x26:
                break;
            default:
                goto done;
            }
        }
    }
    if (!ended)
        goto done;
    i = 0;
    while (i < n)
    {
        unsigned op = p[i++];
        if (op >= 128)
        {
            int32_t offset;
            int64_t target;
            length = (size_t)p[i] | (size_t)p[i + 1] << 8;
            i += 2;
            end = i + length;
            if (op == 0x99 || op == 0x9d)
            {
                offset = (int32_t)p[i] | (int32_t)p[i + 1] << 8;
                if (offset >= 32768)
                    offset -= 65536;
                target = (int64_t)end + offset;
                if (target < 0 || (uint64_t)target >= n || !starts[(size_t)target])
                    goto done;
            }
            i = end;
        }
    }
    ok = 1;
done:
    free(starts);
    return ok;
}
SwlContext *swl_create(const SwlOptions *options)
{
    SwlContext *c = (SwlContext *)calloc(1, sizeof(*c));
    if (!c)
        return NULL;
    c->options.width = 550;
    c->options.height = 400;
    c->options.frames_per_second = 12;
    if (options)
        c->options = *options;
    if (!c->options.width || c->options.width > 3276 || !c->options.height ||
        c->options.height > 3276 || !c->options.frames_per_second ||
        c->options.frames_per_second > 255)
    {
        free(c);
        return NULL;
    }
    return c;
}
void swl_destroy(SwlContext *c)
{
    size_t i;
    if (!c)
        return;
    for (i = 0; i < c->count; ++i)
        free(c->objects[i].data);
    for (i = 0; i < c->asset_count; ++i)
        free(c->assets[i].item.data);
    free(c);
}
int swl_add_object(SwlContext *c, const void *data, size_t n, char *e, size_t cap)
{
    const unsigned char *p = (const unsigned char *)data;
    Item item;
    uint32_t name, code, flags, hash = UINT32_C(2166136261);
    size_t i;
    if (!c || !p || n < 24 || n > SWL_LIMIT || memcmp(p, "SWO1", 4) || swl_u32(p + 4) != 1)
        return swl_fail(e, cap, "invalid SWO1 header/version");
    flags = swl_u32(p + 8);
    name = swl_u32(p + 12);
    code = swl_u32(p + 16);
    if (flags > 1 || !name || name > 255 || name > n - 24 || code != n - 24 - name)
        return swl_fail(e, cap, "invalid SWO1 lengths/flags");
    for (i = 24; i < n; ++i)
        hash = (hash ^ p[i]) * UINT32_C(16777619);
    if (hash != swl_u32(p + 20))
        return swl_fail(e, cap, "SWO1 checksum mismatch");
    for (i = 0; i < name; ++i)
    {
        unsigned ch = p[24 + i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' ||
              (i && ((ch >= '0' && ch <= '9') || ch == '.'))))
            return swl_fail(e, cap, "invalid module name");
    }
    memcpy(item.name, p + 24, name);
    item.name[name] = 0;
    for (i = 0; i < c->count; ++i)
        if (!strcmp(c->objects[i].name, item.name))
            return swl_fail(e, cap, "duplicate module name");
    if (c->count == SWL_ITEMS || n > SWL_LIMIT - c->bytes)
        return swl_fail(e, cap, "link input limit exceeded");
    if (!actions(p + 24 + name, code))
        return swl_fail(e, cap, "invalid AVM1 records or branch targets");
    item.data = (unsigned char *)malloc(code);
    if (!item.data)
        return swl_fail(e, cap, "out of memory");
    memcpy(item.data, p + 24 + name, code);
    item.size = code;
    item.flags = flags;
    c->objects[c->count++] = item;
    c->bytes += n;
    return 1;
}
/* Fixed 17-bit signed RECT fields cover the validated stage size in twips. */
void swl_rect(Buffer *b, unsigned width, unsigned height)
{
    unsigned char rect[10] = {0};
    unsigned fields[5] = {17, 0, width * 20, 0, height * 20};
    unsigned lengths[5] = {5, 17, 17, 17, 17}, bit = 0, i, j;
    for (i = 0; i < 5; ++i)
        for (j = lengths[i]; j > 0; --j, ++bit)
            rect[bit / 8] |= (unsigned char)(((fields[i] >> (j - 1)) & 1) << (7 - bit % 8));
    swl_bytes(b, rect, (bit + 7) / 8);
}
int swl_emit(SwlContext *c, SwlWriter writer, void *user, char *e, size_t cap)
{
    Buffer b = {0};
    size_t i;
    int ok;
    if (!c || !writer || (!c->count && !c->asset_count))
        return swl_fail(e, cap, "no objects or invalid output callback");
    for (i = 0; i < c->count; ++i)
        if (c->objects[i].flags && !c->options.allow_flashvm_extensions)
            return swl_fail(e, cap,
                            "object requires FlashVM channels; enable extensions explicitly");
    swl_bytes(&b, "FWS\010", 4);
    swl_dword(&b, 0);
    swl_rect(&b, c->options.width, c->options.height);
    swl_word(&b, c->options.frames_per_second << 8);
    swl_word(&b, 1);
    swl_resources(c, &b);
    for (i = 0; i < c->count; ++i)
        swl_tag(&b, 12, c->objects[i].data, c->objects[i].size);
    swl_tag(&b, 1, NULL, 0);
    swl_tag(&b, 0, NULL, 0);
    if (b.failed)
    {
        free(b.data);
        return swl_fail(e, cap, "SWF size/allocation limit exceeded");
    }
    for (i = 0; i < 4; ++i)
        b.data[4 + i] = (unsigned char)(b.size >> (8 * i));
    ok = writer(user, b.data, b.size);
    free(b.data);
    return ok ? 1 : swl_fail(e, cap, "output callback failed");
}
