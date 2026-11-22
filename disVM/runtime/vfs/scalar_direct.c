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
_Static_assert(offsetof(DirectRefPairFrame, objects) == 64 && sizeof(DirectRefPairFrame) == 80,
    "compiled NC64 ref-pair frame");
_Static_assert(offsetof(DirectCallListFrame, arguments) == 64 && sizeof(DirectCallListFrame) == 96,
    "compiled NC64 argument-list frame");


/* EN/RU: See scalar_direct.h. / См. scalar_direct.h. */
int dvm_styx_frame_layout(const char *shape, size_t *words, unsigned char *pointers)
{
    StyxScalarLayout layout;
    if (!styxmanaged_layout(shape, &layout) || layout.size > 8 * sizeof(WORD))
    {
        return 0;
    }
    unsigned char mask = 0;
    for (size_t i = 0; i < layout.count; i++)
    {
        if (layout.pointers & (1U << i))
        {
            mask |= (unsigned char)(0x80U >> (layout.offset[i] / sizeof(WORD)));
        }
    }
    *words = (layout.size + sizeof(WORD) - 1) / sizeof(WORD);
    *pointers = mask;
    return 1;
}


/* EN/RU: See scalar_direct.h. / См. scalar_direct.h. */
size_t dvm_styx_call_code(const char *shape, unsigned slot, int result, Inst *code, size_t capacity)
{
    StyxScalarLayout layout;
    const unsigned module = 64;
    const unsigned arguments = 72;
    const unsigned returned = 32;
    if (!code || slot > INT16_MAX || !styxmanaged_layout(shape, &layout) ||
        capacity < layout.count + 3 + !!result)
    {
        return 0;
    }
    ushort temporary = (ushort)(arguments + (layout.size + 7) / 8 * 8);
    size_t n = 0;
    code[n++] = (Inst){.op = IMFRAME, .add = SRC(AFP) | AXIMM | DST(AFP),
        .reg = (ushort)slot, .s.ind = module, .d.ind = temporary};
    for (size_t i = 0; i < layout.count; i++)
    {
        unsigned offset = (unsigned)layout.offset[i];
        uchar operation = IMOVL;
        if (layout.pointers & (1U << i))
        {
            operation = IMOVP;
        }
        else if (layout.width[i] == 1)
        {
            operation = IMOVB;
        }
        code[n++] = (Inst){.op = operation, .add = SRC(AFP) | DST(AFP | AIND),
            .s.ind = arguments + offset, .d.i = {temporary, (ushort)(module + offset)}};
    }
    if (result)
    {
        code[n++] = (Inst){.op = ILEA, .add = SRC(AFP | AIND) | DST(AFP | AIND),
            .s.i = {returned, 0}, .d.i = {temporary, returned}};
    }
    code[n++] = (Inst){.op = IMCALL, .add = SRC(AFP) | AXIMM | DST(AFP),
        .reg = (ushort)slot, .s.ind = temporary, .d.ind = module};
    code[n++] = (Inst){.op = IRET, .add = SRC(AXXX) | DST(AXXX)};
    return n;
}


typedef struct CallModuleBuild
{
    Module *module; /* Owned unpublished result during assembly. */
    const Inst *code; /* Borrowed generated instructions until copied. */
    size_t count; /* Number of instructions. */
    size_t words; /* Number of argument words in the stub. */
    unsigned char pointers; /* Reference mask for the original argument area. */
    uint32_t signature; /* Compiled signature of the caller's private interface. */
} CallModuleBuild;


/* EN: Allocate text, frame descriptor and export. Leave partial state for unload.
 * RU: Выделить код, дескриптор кадра и экспорт. Передать очистку остатков unload. */
static int build_call_module(void *opaque)
{
    CallModuleBuild *build = opaque;
    Module *module = build->module = newmod("$StyxCall");
    module->prog = calloc(build->count, sizeof(*module->prog));
    module->ext = calloc(2, sizeof(*module->ext));
    module->type = calloc(1, sizeof(*module->type));
    if (!module->prog || !module->ext || !module->type)
    {
        return 0;
    }
    unsigned char map[] = {0, 0x80 | build->pointers >> 1, (unsigned char)(build->pointers << 7)};
    size_t frame_size = (10 + build->words) * sizeof(WORD);
    Type *type = dtype(freeheap, frame_size, map, map[2] ? sizeof(map) : sizeof(map) - 1);
    if (!type)
    {
        return 0;
    }
    module->type[module->ntype++] = type;
    module->ext[0].name = strdup("invoke");
    if (!module->ext[0].name)
    {
        return 0;
    }
    memcpy(module->prog, build->code, build->count * sizeof(*module->prog));
    module->nprog = (int)build->count;
    module->ext[0].sig = (int32_t)build->signature;
    module->ext[0].frame = type;
    module->ext[0].u.pc = module->prog;
    return 1;
}


/* EN/RU: See scalar_direct.h. / См. scalar_direct.h. */
Module *dvm_styx_call_module(const char *shape, unsigned slot, int result, uint32_t signature)
{
    Inst code[STYX_CALL_VALUES + 4];
    CallModuleBuild build = {.code = code, .signature = signature};
    build.count = dvm_styx_call_code(shape, slot, result, code, sizeof(code) / sizeof(code[0]));
    if (!signature || !build.count || !dvm_styx_frame_layout(shape, &build.words, &build.pointers))
    {
        return NULL;
    }
    if (!kprotect(build_call_module, &build))
    {
        if (build.module)
        {
            unload(build.module);
        }
        return NULL;
    }
    return build.module;
}


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
    size_t count = 1;
    for (DVM_ScalarFiles *entry = build->files; entry && entry->next; entry = entry->next)
    {
        count++;
    }
    module->type = calloc(count, sizeof(*module->type));
    module->ext = calloc(count + 1, sizeof(*module->ext));
    if (!module->type || !module->ext)
    {
        return 0;
    }
    Type *frames[5] = {0};
    const size_t sizes[] = {sizeof(DirectFrame), sizeof(DirectTupleFrame), sizeof(DirectRefFrame),
        sizeof(DirectCallListFrame), sizeof(DirectRefPairFrame)};
    DVM_ScalarFiles *entry = build->files;
    for (size_t i = 0; i < count; i++)
    {
        char *name = entry && entry->duplex ? entry->name : build->name;
        uint32_t signature = entry && entry->duplex ? entry->signature : DVM_SCALAR_INT_SIGNATURE;
        if (entry && entry->dynamic)
        {
            size_t words;
            uchar map[] = {0, 0};
            if (!dvm_styx_frame_layout(entry->shape.arguments, &words, &map[1]))
            {
                return 0;
            }
            size_t size = offsetof(DirectCallListFrame, arguments) + words * sizeof(WORD);
            Type *type = dtype(freeheap, size, map[1] ? map : NULL, map[1] ? sizeof(map) : 0);
            if (!type)
            {
                return 0;
            }
            module->type[module->ntype++] = type;
            runtime(module, &module->ext[i], name, (int32_t)signature, scalar_slots[i], type);
            entry = entry->next;
            continue;
        }
        unsigned long ref_profile = entry ? entry->ref_profile : 0;
        size_t profile = signature == DVM_REF_PAIR_SIGNATURE || ref_profile == STYX_REF_PAIR_REQUEST_TAG ? 4 :
            signature == DVM_CALL_LIST_SIGNATURE ? 3 :
            signature == DVM_REF_SIGNATURE || ref_profile == STYX_REF_REQUEST_TAG ? 2 :
            signature == DVM_TUPLE_SIGNATURE ? 1 : 0;
        if (!frames[profile])
        {
            uchar map[] = {0, profile == 4 ? 0xc0 : 0x80};
            int mapped = profile == 2 || profile == 4;
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
