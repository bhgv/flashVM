/* Private helpers shared by selected OS filesystem implementations. */
#ifndef FVM_FILESYSTEM_COMMON_H
#define FVM_FILESYSTEM_COMMON_H
#include "filesystem_native.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Split an exact native bit pattern into the C89 public representation. */
static FVM_FileNumber64 fs_number(uint64_t value)
{
    FVM_FileNumber64 result;
    result.high = (unsigned long)(value >> 32);
    result.low = (unsigned long)(value & 0xffffffffUL);
    return result;
}


/* Validate a NUL-terminated UTF-8 name without linking a VM/allocator dependency.
 * Borrow text; return false for overlong sequences, surrogates or invalid scalars. */
static int fs_name_valid(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    while (*p)
    {
        unsigned long value;
        unsigned long minimum;
        int remaining;
        if (*p < 0x80)
        {
            p++;
            continue;
        }
        if (*p >= 0xc2 && *p <= 0xdf)
        {
            value = *p++ & 0x1f;
            remaining = 1;
            minimum = 0x80;
        }
        else if (*p >= 0xe0 && *p <= 0xef)
        {
            value = *p++ & 0x0f;
            remaining = 2;
            minimum = 0x800;
        }
        else if (*p >= 0xf0 && *p <= 0xf4)
        {
            value = *p++ & 7;
            remaining = 3;
            minimum = 0x10000;
        }
        else
        {
            return 0;
        }
        while (remaining--)
        {
            if (*p < 0x80 || *p > 0xbf)
            {
                return 0;
            }
            value = (value << 6) | (*p++ & 0x3f);
        }
        if (value < minimum || value > 0x10ffff || (value >= 0xd800 && value <= 0xdfff))
        {
            return 0;
        }
    }
    return 1;
}


/* Copy a complete UTF-8 basename; reject unsupported lengths rather than truncate. */
static int fs_name(char *output, const char *name)
{
    size_t length = strlen(name);
    while (length > 1 && (name[length - 1] == '/'
#ifdef _WIN32
                          || name[length - 1] == '\\'
#endif
                          ))
    {
        length--;
    }
    if (length >= FVM_FS_NAME_MAX || !fs_name_valid(name))
    {
        return 0;
    }
    memcpy(output, name, length);
    output[length] = 0;
    return 1;
}


/* Locate the last component of a borrowed UTF-8 filesystem path. */
static const char *fs_basename(const char *path)
{
    const char *name = path;
    const char *p;
    for (p = path; *p; p++)
    {
        int separator = *p == '/';
#ifdef _WIN32
        separator = separator || *p == '\\';
#endif
        if (separator)
        {
            const char *next = p;
            while (*next == '/'
#ifdef _WIN32
                   || *next == '\\'
#endif
            )
            {
                next++;
            }
            if (*next)
            {
                name = next;
            }
        }
    }
    return name;
}
#endif
