/* Copyright (c) 2026 bhgv
 * EN: Shared ABI for a native scalar export frame.
 * RU: Общий ABI нативного кадра скалярного экспорта.
 */
#ifndef DVM_SCALAR_DIRECT_H
#define DVM_SCALAR_DIRECT_H
#include <stdint.h>
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

typedef struct DirectManagedFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    void *ret; /* Borrowed result slot, possibly mapped. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    WORD arguments[8]; /* Bounded dynamic scalar and managed-value leaves. */
} DirectManagedFrame;

typedef struct DirectRefPairFrame
{
    WORD regs[NREG-1]; /* VM linkage cells. */
    void *ret; /* Borrowed two-cell return destination. */
    unsigned char temps[24]; /* Compiler ABI temporary area. */
    void *objects[2]; /* Two rooted references, possibly aliased. */
} DirectRefPairFrame;

/* EN: Derive the NC64 argument extent and reference bits from a checked shape.
 * RU: Вычислить размер аргументов NC64 и карту ссылок по проверенной схеме. */
int dvm_styx_frame_layout(const char *shape, size_t *words, unsigned char *pointers);

/* EN: Emit a Dis call and return for a validated argument shape. The caller
 * frame has the target at byte 64, then arguments and one raw frame temporary.
 * RU: Сгенерировать вызов и возврат Dis. В кадре по смещению 64 находятся
 * модуль, далее аргументы и один временный слот указателя на кадр. */
size_t dvm_styx_call_code(const char *shape, unsigned slot, int result, Inst *code, size_t capacity);

/* EN: Emit a call with the target at module_offset in the bridge frame.
 * RU: Выдать вызов с целью по смещению module_offset в кадре шлюза. */
size_t dvm_styx_call_code_at(const char *shape, unsigned slot, int result, Inst *code,
    size_t capacity, unsigned module_offset);


/* EN: Build a private interpreted module containing one generated call export.
 * The caller owns the returned module or receives NULL on failure.
 * RU: Собрать приватный интерпретируемый модуль с одним экспортом.
 * Вызывающий владеет результатом либо получает NULL при отказе. */
Module *dvm_styx_call_module(const char *shape, unsigned slot, int result, uint32_t signature);

/* EN: Match a borrowed export frame against the NC64 argument map.
 * RU: Сверить кадр экспорта с картой аргументов NC64. */
int dvm_styx_export_frame(const Type *frame, size_t arguments, unsigned char references);

/* EN: Borrow target and shape, return an owned gateway module or NULL.
 * RU: Заимствовать target и shape, вернуть собственный модуль шлюза или NULL. */
struct StyxCallShape;
Module *dvm_styx_owner_gateway(Modlink *target, unsigned slot, const struct StyxCallShape *shape,
    uint32_t gateway_signature);

struct DVM_ScalarFiles;

/* EN: Transfer two mapped references with their identity map.
 * RU: Передать две ссылки из карты кадра с сохранением идентичности. */
void dvm_ref_pair_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);

/* EN: Dispatch the compiled pointer-free argument-list frame on files.
 * RU: Вызвать скомпилированный кадр списка без ссылок через files. */
void dvm_call_list_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);


/* EN: Dispatch a compiled managed-value frame through one mounted function fid.
 * RU: Вызвать скомпилированный кадр управляемых значений через fid функции.
 */
void dvm_managed_call_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);

/* EN: Dispatch one compiled composed frame. / RU: Вызвать один составной кадр. */
void dvm_tuple_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);
/* EN: Dispatch a mapped ref frame. / RU: Вызвать кадр с ref. */
void dvm_ref_entry(void *frame, Type *expected_type, struct DVM_ScalarFiles *files);
#endif
