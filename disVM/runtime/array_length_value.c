/* No dereference beyond the validated length field; preserve native WORD bits. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "array_length_value.h"


/* source is a complete guarded pointer slot. Return its bits as a borrowed
 * pointer, with no dereference, ownership transfer, allocation or callback. */
void *dvm_runtime_array_length_source(const void *source)
{
    void *value;
    memmove(&value, source, sizeof(value));
    return value;
}


/* Borrow guarded field/destination, with NULL field denoting nil/H. Return void
 * after one snapshot/store; no callbacks, allocation or reference transfer. */
void dvm_runtime_array_length_value(const void *field, void *destination)
{
    WORD length = 0;
    if (field)
    {
        memmove(&length, field, sizeof(length));
    }
    memmove(destination, &length, sizeof(length));
}
