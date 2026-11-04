/* Alignment-safe snapshots, separated from managed admission and control flow. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "string_branch_value.h"
#include <stddef.h>


/* source borrows a guarded complete pointer cell. Return its bits without
 * touching the referent, transferring ownership, allocating or calling the VM. */
void *dvm_runtime_string_branch_source(const void *source)
{
    void *value;
    memmove(&value, source, sizeof(value));
    return value;
}


/* object borrows a guarded len/max span; length/capacity are local int outputs.
 * No payload/GC access, validation, allocation, callback or ownership change. */
void dvm_runtime_string_branch_header(const void *object, int *length, int *capacity)
{
    memmove(length, (const unsigned char *)object+offsetof(String, len), sizeof(*length));
    memmove(capacity, (const unsigned char *)object+offsetof(String, max), sizeof(*capacity));
}


/* Borrow the guarded prefix and validated index/representation. Return one key
 * without alignment assumptions, callbacks, allocations or owner changes. */
uint32_t dvm_runtime_string_branch_key(const void *data, size_t index, int wide)
{
    if (wide)
    {
        Rune value;
        memmove(&value, (const unsigned char *)data+index*sizeof(value), sizeof(value));
        return (uint32_t)value;
    }
    return ((const unsigned char *)data)[index];
}
