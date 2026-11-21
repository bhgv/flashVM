/* Copyright (c) 2026 bhgv
 * EN: Shared ABI for a native scalar export frame.
 * RU: Общий ABI нативного кадра скалярного экспорта.
 */
#ifndef DVM_SCALAR_DIRECT_H
#define DVM_SCALAR_DIRECT_H
typedef struct DirectFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    WORD *ret; /* Borrowed result slot. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    WORD value; /* One scalar argument. */
} DirectFrame;

typedef struct DirectTupleFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    WORD *ret; /* Borrowed two-cell result slot. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    WORD narrow; /* Signed int in the NC64 cell. */
    LONG wide; /* Signed big in the adjacent cell. */
} DirectTupleFrame;


/* EN/RU: Dispatch a compiled frame through its native export slot.
 * Передать компилированный кадр через слот нативного экспорта. */
void dvm_scalar_entry(void *frame, unsigned index);

struct DVM_ScalarFiles;

/* EN: Dispatch one compiled composed frame. / RU: Вызвать один составной кадр. */
void dvm_tuple_entry(void *frame, Module *module, struct DVM_ScalarFiles *files);
#endif
