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

typedef struct DirectRefFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    void *ret; /* Borrowed two-cell return destination. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    void *object; /* Mapped borrowed reference. */
} DirectRefFrame;

typedef struct DirectCallListFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    WORD *ret; /* Borrowed three-cell result. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    WORD arguments[4]; /* Two scalar parameters and two fields in the tuple parameter. */
} DirectCallListFrame;

struct DVM_ScalarFiles;

/* EN: Dispatch the compiled pointer-free argument-list frame on files.
 * RU: Вызвать скомпилированный кадр списка без ссылок через files. */
void dvm_call_list_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);

/* EN: Dispatch one compiled composed frame. / RU: Вызвать один составной кадр. */
void dvm_tuple_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);
/* EN: Dispatch a mapped ref frame. / RU: Вызвать кадр с ref. */
void dvm_ref_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);
#endif
