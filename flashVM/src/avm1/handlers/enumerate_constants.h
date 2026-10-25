/* FlashVM: roots and traversal policy for value-based property enumeration. */
#ifndef FVM_EXEC_ENUMERATE_CONSTANTS_H
#define FVM_EXEC_ENUMERATE_CONSTANTS_H          /* Enumeration service guard. */
#define FVM_EXEC_ENUMERATE_INPUT_ROOT 0U        /* [GC:] Original object retained during key snapshot. */
#define FVM_EXEC_ENUMERATE_ROOT_COUNT 1U        /* [GC:] One retained input; native snapshot cannot call script. */
#define FVM_EXEC_ENUMERATE_INCLUDE_PROTOTYPES 1 /* Include visible inherited names, hiding duplicates. */
#endif
