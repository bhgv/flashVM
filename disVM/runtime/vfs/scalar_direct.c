/* Copyright (c) 2026 bhgv
 * EN: Build native scalar Dis link slots from verified mounted grants.
 * RU: Построение нативных слотов Dis из проверенных смонтированных разрешений.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "loader.h"
#include "scalar_files.h"
#include "scalar_direct.h"
#include <stdlib.h>

_Static_assert(sizeof(LONG) == sizeof(WORD), "mounted big and int use the same NC64 frame cell");
_Static_assert(offsetof(DirectRefFrame, object) == 64, "compiled NC64 ref map");
_Static_assert(offsetof(DirectCallListFrame, arguments) == 64 && sizeof(DirectCallListFrame) == 96,
    "compiled NC64 argument-list frame");


/* EN/RU: Give each native Link its own stable route ordinal.
 * Назначить каждому нативному Link постоянный номер маршрута. */
#define SCALAR_SLOT(index) \
    static void scalar_slot_##index(void *frame) \
    { \
        dvm_scalar_entry(frame, index); \
    }
SCALAR_SLOT(0)
SCALAR_SLOT(1)
SCALAR_SLOT(2)
SCALAR_SLOT(3)
SCALAR_SLOT(4)
SCALAR_SLOT(5)
SCALAR_SLOT(6)
SCALAR_SLOT(7)
#undef SCALAR_SLOT

static void (*const scalar_slots[STYX_MOUNTED_EXPORT_LIMIT])(void *) = {
    scalar_slot_0, scalar_slot_1, scalar_slot_2, scalar_slot_3,
    scalar_slot_4, scalar_slot_5, scalar_slot_6, scalar_slot_7
}; /* Export trampolines in captured-route order. */


typedef struct DirectBuild
{
    char *path; /* Borrowed logical path through construction. */
    char *name; /* Borrowed checked import name. */
    Module *module; /* Owned partial allocation through kprotect. */
    DVM_ScalarFiles *files; /* Borrowed mounted export chain, if present. */
} DirectBuild;


/* EN/RU: Construct one private native module with the compiler's scalar frame.
 * Создать собственный нативный модуль с компилированным скалярным кадром. */
static int build_direct(void *opaque)
{
    DirectBuild *build = opaque;
    Module *module = build->module = newmod(build->path);
    module->type = calloc(4, sizeof(*module->type));
    size_t count = 1;
    for (DVM_ScalarFiles *entry = build->files; entry && entry->next; entry = entry->next)
    {
        count++;
    }
    module->ext = calloc(count + 1, sizeof(*module->ext));
    if (!module->type || !module->ext)
    {
        return 0;
    }
    Type *frames[4] = {0};
    const size_t sizes[] = {sizeof(DirectFrame), sizeof(DirectTupleFrame), sizeof(DirectRefFrame),
        sizeof(DirectCallListFrame)};
    DVM_ScalarFiles *entry = build->files;
    for (size_t i = 0; i < count; i++)
    {
        char *name = entry && entry->duplex ? entry->name : build->name;
        uint32_t signature = entry && entry->duplex ? entry->signature : DVM_SCALAR_INT_SIGNATURE;
        size_t profile = signature == DVM_CALL_LIST_SIGNATURE ? 3 :
            signature == DVM_REF_SIGNATURE ? 2 : signature == DVM_TUPLE_SIGNATURE ? 1 : 0;
        if (!frames[profile])
        {
            uchar map[] = {0, 0x80}; /* Compiler map for the ref pointer at byte 64. */
            int mapped = signature == DVM_REF_SIGNATURE;
            frames[profile] = dtype(freeheap, sizes[profile], mapped ? map : NULL, mapped ? sizeof(map) : 0);
            if (!frames[profile])
            {
                return 0;
            }
            module->type[module->ntype++] = frames[profile];
        }
        runtime(module, &module->ext[i], name, (int32_t)signature, scalar_slots[i], frames[profile]);
        entry = entry ? entry->next : NULL;
    }
    return 1;
}


/* EN/RU: Return a private noncached module and reuse per-instance namespace attachment.
 * Вернуть приватный модуль без кэша и использовать привязку пространства экземпляра. */
Module *dvm_scalar_module(char *path, char *name, unsigned long request_tag,
    unsigned long reply_tag, uint64_t revision, DVM_ScalarFiles *files)
{
    DirectBuild build = {path, name, NULL, files};
    if (!kprotect(build_direct, &build))
    {
        if (build.module)
        {
            unload(build.module);
        }
        kwerrstr("cannot construct remote scalar module");
        return NULL;
    }
    if (!dvm_scalar_files_retain(files))
    {
        unload(build.module);
        kwerrstr("cannot retain scalar file grants");
        return NULL;
    }
    build.module->scalar_files = files;
    /* Private noncached descriptor: these fields carry namespace revision and route tags. */
    build.module->qid.type = DVM_MODULE_ADAPTER_QID;
    build.module->qid.path = revision;
    build.module->qid.vers = reply_tag;
    build.module->dev = request_tag;
    return build.module;
}
