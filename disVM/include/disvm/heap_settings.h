/* Hosted heap identity settings. Zero is reserved for an invalid generation. */
#ifndef DVM_HEAP_SETTINGS_H
#define DVM_HEAP_SETTINGS_H
#include <stdint.h>

typedef uint64_t DVM_HeapGeneration;

/* Local identity. The numeric address grants no access to the allocation. */
typedef struct DVM_HeapKey
{
    uintptr_t address;
    DVM_HeapGeneration generation;
} DVM_HeapKey;

#endif
