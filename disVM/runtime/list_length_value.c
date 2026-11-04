/* Complete pointer/WORD snapshots; no payload/type/FP or allocation effects. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "list_length_value.h"


/* Borrow guarded cell and return pointer bits without dereferencing a referent.
 * No allocation, ownership/root transfer, safepoint or callback. */
void *dvm_runtime_list_length_pointer(const void *cell)
{
    void *value;
    memmove(&value, cell, sizeof(value));
    return value;
}


/* Borrow writable destination; count is in0..WORD_MAX. Publish its native WORD
 * representation, returning void with no callback, allocation or owner change. */
void dvm_runtime_list_length_value(uint64_t count, void *destination)
{
    WORD value = (WORD)count;
    memmove(destination, &value, sizeof(value));
}
