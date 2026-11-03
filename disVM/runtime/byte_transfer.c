/* Generic data transfer, separate from native admission and IDE styling. */
#include "byte_transfer.h"
#include <string.h>


/* Caller guarantees mapped spans/lifetimes and complete exclusion. No callback. */
void dvm_runtime_byte_transfer(void *destination, const void *source, size_t size)
{
    memmove(destination, source, size);
}
