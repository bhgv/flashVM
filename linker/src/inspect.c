/* Context-free validation and metadata visitation for IDEs and command clients.
 * Reuse the linker reader so inspection cannot accept a different SWO dialect. */
#include "internal.h"
#include <string.h>


/* Validate everything before publishing even the first borrowed module name. */
int swl_inspect(const void *data, size_t size, SwlModuleVisitor visitor, void *user, char *error, size_t capacity)
{
    const unsigned char *bytes = (const unsigned char *)data;
    SwlContext *context;
    size_t i;
    int ok;

    if (!bytes || size < 4 || !visitor)
    {
        return swl_fail(error, capacity, "invalid inspection arguments");
    }
    context = swl_create(NULL);
    if (!context)
    {
        return swl_fail(error, capacity, "out of memory");
    }
    if (!memcmp(bytes, "SWA1", 4))
    {
        ok = swl_add_archive(context, bytes, size, error, capacity);
    }
    else
    {
        ok = swl_add_object(context, bytes, size, error, capacity);
    }
    if (ok)
    {
        for (i = 0; i < context->count; ++i)
        {
            SwlModuleInfo info;
            info.name = context->objects[i].name;
            info.code_size = context->objects[i].size;
            info.requirements = context->objects[i].flags;
            if (!visitor(user, &info))
            {
                ok = swl_fail(error, capacity, "inspection callback cancelled");
                break;
            }
        }
    }
    swl_destroy(context);
    return ok;
}
