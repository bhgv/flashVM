/* No dereference beyond the validated length field; preserve native signed int representation. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "string_length_value.h"


/* source is a complete guarded pointer slot. Return its bits as a borrowed
 * pointer, with no dereference, ownership transfer, allocation or callback. */
void *dvm_runtime_string_length_source(const void *source)
{
    void *value;
    memmove(&value, source, sizeof(value));
    return value;
}


/* Borrow a guarded int field and WORD destination; NULL field denotes H.
 * Return0 for INT_MIN without publishing,1 after sign normalization and store.
 * No payload reads, callbacks, allocation, transfer or unguarded owner access. */
int dvm_runtime_string_length_value(const void *field, void *destination)
{
    int encoded = 0;
    WORD length;
    if (field)
    {
        memmove(&encoded, field, sizeof(encoded));
    }
    if (encoded == (-0x7fffffff-1))
    {
        return 0;
    }
    length = encoded < 0 ? -encoded : encoded;
    memmove(destination, &length, sizeof(length));
    return 1;
}
