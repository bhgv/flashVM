/* Copyright (c) 2026 bhgv
 * EN: Local identities and staged copy-back for the ordinary Dis managed ref profile.
 * RU: Локальные идентификаторы и подготовка обратной записи обычного профиля ref Dis.
 */
#ifndef FLASHVM_STYXRPC_MANAGED_REFS_H
#define FLASHVM_STYXRPC_MANAGED_REFS_H
#include "flashvm/gc_roots.h"
#include "styxrpc/managed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FVM_STYXRPC_REF_SLOTS 8U  /* Ordinary Dis ref argument mask width. */
#define FVM_STYXRPC_REF_FIELDS 8U /* Maximum fields staged per plain ADT shell. */

typedef struct FVM_StyxRef
{
    const char *type;      /* Borrowed exact ref token in the current schema. */
    FVM_Object *object;    /* Rooted same-context native Array. */
    FVM_SRecord before;    /* Borrowed original field bytes, without the ref index. */
    FVM_Value original[FVM_STYXRPC_REF_FIELDS]; /* Rooted live slots before submission. */
    FVM_Value updated[FVM_STYXRPC_REF_FIELDS];  /* Rooted staged copy-back. */
    unsigned count;        /* Fixed number of own fields. */
    unsigned changed;      /* Field mask prepared from the checked reply. */
    unsigned slot;         /* First ref argument position of this identity. */
} FVM_StyxRef;

typedef struct FVM_StyxRefs
{
    FVM_StyxRef definitions[FVM_STYXRPC_REF_SLOTS]; /* Native object indexed by wire identity. */
    unsigned count;        /* Number of distinct IDs. */
    unsigned slots;        /* Number of ref positions visited, including nil and aliases. */
    unsigned char *storage; /* Owned client baseline storage; NULL on server. */
    size_t used;           /* Stored baseline byte extent. */
} FVM_StyxRefs;


/* [GC:] EN: Read exact own native Array slots without callbacks.
 * RU: Прочитать точные собственные слоты нативного Array без колбэков. */
int fvm_styxrpc_ref_snapshot(FVM_Context *context, FVM_Object *object, unsigned count, FVM_Value *values);


/* [GC:] EN: Verify all destination slots, then apply staged changes in a nonallocating, callback-free pass.
 * RU: Сверить все слоты назначения, затем применить изменения без выделения и колбэков. */
int fvm_styxrpc_refs_commit(FVM_Context *context, FVM_StyxRefs *refs);


/* [GC:] EN: Encode a nil, first definition or alias; retain baseline for copy-back.
 * RU: Закодировать nil, первое определение или алиас и удержать исходные поля. */
int fvm_styxrpc_ref_encode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used);


/* [GC:] EN: Decode a nil, first definition or alias into a rooted local reference.
 * RU: Декодировать nil, первое определение или алиас в локальную ссылку под корнем. */
int fvm_styxrpc_ref_decode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const char *type, const FVM_SRecord *node, FVM_Value *value);


/* [GC:] EN: Encode changed definitions in first-argument order, skipping aliases and nil.
 * RU: Закодировать изменённые определения в порядке аргументов, пропуская алиасы и nil. */
int fvm_styxrpc_refs_reply(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    unsigned char *bytes, size_t capacity, StyxRpcOutput *output);


/* [GC:] EN: Decode validated updates and preserve unchanged local fields.
 * RU: Декодировать проверенные изменения и сохранить неизменившиеся локальные поля. */
int fvm_styxrpc_refs_stage(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcManagedReply *reply);

#ifdef __cplusplus
}
#endif
#endif
