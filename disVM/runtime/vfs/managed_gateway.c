/* Copyright (c) 2026 bhgv
 * EN: Bind a private interpreted bridge to an original export and its compiled types.
 * RU: Связывание частного шлюза Dis с исходным экспортом и его типами.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "scalar_direct.h"
#include "managed_value.h"
#include "raise.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>

enum
{
    GATE_WIRE = 64, /* One array of byte argument from the owner dispatcher. */
    GATE_TARGET = 72, /* Start of the generated call's mapped arguments. */
    GATE_SIGNATURE = 1 /* Internal identity shared by the private exports and their linker. */
};

typedef struct GatewayData
{
    Modlink *target; /* Owned original export at slot zero. */
    Modlink *helpers; /* Owned native prepare and finish slots. */
    StyxCallShape shape; /* Compiled schema, immutable after publication. */
    size_t result_offset; /* Mapped result storage in the parent bridge frame. */
    size_t result_size; /* Native result extent. */
    size_t extent; /* Bridge frame size. */
} GatewayData;


/* [GC:] EN: Decode the rooted request and move its values into the parent Dis frame.
 * RU: Декодировать удерживаемый запрос и передать значения в родительский кадр Dis. */
static void prepare_call(void *frame)
{
    Frame *helper = frame;
    GatewayData *data = (GatewayData *)helper->mr->MP;
    unsigned char *bridge = helper->fp;
    Array *wire;
    memcpy(&wire, bridge + GATE_WIRE, sizeof(wire));
    DVM_StyxValueFrame arguments = {0};
    if (!wire || wire == H || wire->t != &Tbyte || wire->len <= 0 || wire->len > STYX_MANAGED_LIST_LIMIT ||
        (!dvm_styx_value_frame_read(STYX_MANAGED_REQUEST_TAG, data->shape.arguments,
            wire->data, (size_t)wire->len, &arguments) &&
         !dvm_styx_value_frame_read(STYX_MANAGED_DELTA_REQUEST_TAG, data->shape.arguments,
            wire->data, (size_t)wire->len, &arguments)))
    {
        error("invalid managed owner arguments");
    }
    if (!dvm_styx_owner_frame_init(&arguments, data->target, bridge + 8, data->extent - 8))
    {
        dvm_styx_value_frame_clear(&arguments);
        error("cannot install managed owner arguments");
    }
}


/* [GC:] EN: Serialize the rooted return area while its parent Dis frame is live.
 * RU: Кодировать удерживаемый результат пока родительский кадр Dis жив. */
static void finish_call(void *frame)
{
    Frame *helper = frame;
    GatewayData *data = (GatewayData *)helper->mr->MP;
    Array *wire;
    FVM_SRecord baseline;
    memcpy(&wire, helper->fp + GATE_WIRE, sizeof(wire));
    if (!styxmanaged_list_read(STYX_MANAGED_REQUEST_TAG, data->shape.arguments,
        wire->data, (size_t)wire->len, &baseline) &&
        !styxmanaged_list_read(STYX_MANAGED_DELTA_REQUEST_TAG, data->shape.arguments,
            wire->data, (size_t)wire->len, &baseline))
    {
        error("invalid managed owner baseline");
    }
    dvm_styx_owner_finish(&data->shape, helper->fp + GATE_TARGET + 8, &baseline,
        helper->fp + data->result_offset, data->result_size);
}


/* EN: Set the exact pointer bit for one aligned frame cell.
 * RU: Установить точный бит ссылки для выровненной ячейки кадра. */
static void map_slot(unsigned char *map, size_t offset)
{
    size_t word = offset / sizeof(WORD);
    map[word / 8] |= 0x80U >> (word % 8);
}


typedef struct GatewayBuild
{
    GatewayData data; /* Borrowed inputs and computed layout until stored in MP. */
    Module *module; /* Owned unpublished bridge, consumed by linkmod. */
    Module *native; /* Owned unpublished helper module, consumed by linkmod. */
    Modlink *link; /* Owned bridge during protected construction. */
    Modlink *helpers; /* Owned helper instance until transferred into MP. */
    int helper_root; /* Temporary root across MP allocation. */
} GatewayBuild;


/* EN: Allocate descriptors, code and slot links under protected construction.
 * RU: Выделить дескрипторы, код и связи слотов под защитой обработки ошибок. */
static int build_gateway(void *opaque)
{
    GatewayBuild *build = opaque;
    GatewayData *data = &build->data;
    StyxScalarLayout input, output;
    if (!styxmanaged_layout(data->shape.arguments, &input) ||
        !styxmanaged_layout(data->shape.result, &output))
    {
        return 0;
    }
    const unsigned temporary = GATE_TARGET + 8 + (unsigned)((input.size + 7) & ~(size_t)7);
    data->result_offset = temporary + 8;
    data->result_size = output.size;
    data->extent = data->result_offset + ((output.size + 7) & ~(size_t)7);
    unsigned char map[4] = {0};
    map_slot(map, GATE_WIRE);
    map_slot(map, GATE_TARGET);
    for (size_t i = 0; i < input.count; i++)
    {
        if (input.pointers & (1U << i))
        {
            map_slot(map, GATE_TARGET + 8 + input.offset[i]);
        }
    }
    for (size_t i = 0; i < output.count; i++)
    {
        if (output.pointers & (1U << i))
        {
            map_slot(map, data->result_offset + output.offset[i]);
        }
    }
    size_t mapsize = (data->extent + 63) / 64;
    Module *module = build->module = newmod("$StyxManagedCall");
    module->type = calloc(2, sizeof(*module->type));
    module->ext = calloc(2, sizeof(*module->ext));
    module->prog = calloc(STYX_CALL_VALUES + 10, sizeof(*module->prog));
    if (!module->type || !module->ext || !module->prog)
    {
        return 0;
    }
    Type *type = dtype(freeheap, data->extent, map, mapsize);
    if (!type)
    {
        return 0;
    }
    module->type[module->ntype++] = type;
    unsigned char mpmap = 0xc0;
    Type *mp = dtype(freeheap, sizeof(GatewayData), &mpmap, 1);
    if (!mp)
    {
        return 0;
    }
    module->type[module->ntype++] = mp;
    mlink(module, module->ext, (uchar *)"invoke", GATE_SIGNATURE, 0, type);
    Inst *code = module->prog;
    code[0] = (Inst){.op = IMFRAME, .add = SRC(AMP) | AXIMM | DST(AFP),
        .s.ind = offsetof(GatewayData, helpers), .d.ind = temporary};
    code[1] = (Inst){.op = IMCALL, .add = SRC(AFP) | AXIMM | DST(AMP),
        .s.ind = temporary, .d.ind = offsetof(GatewayData, helpers)};
    code[2] = (Inst){.op = ILEA, .add = SRC(AFP) | DST(AFP),
        .s.ind = (ushort)data->result_offset, .d.ind = 32};
    size_t count = dvm_styx_call_code_at(data->shape.arguments, 0, output.size != 0,
        code + 3, STYX_CALL_VALUES + 4, GATE_TARGET);
    if (!count)
    {
        return 0;
    }
    size_t end = count + 2; /* Replace the generated return with the reply call. */
    code[end] = code[0];
    code[end++].reg = 1;
    code[end] = code[1];
    code[end++].reg = 1;
    code[end++] = (Inst){.op = IRET, .add = SRC(AXXX) | DST(AXXX)};
    module->nprog = (int)end;
    Module *native = build->native = newmod("$StyxManagedHelper");
    native->type = calloc(1, sizeof(*native->type));
    native->ext = calloc(3, sizeof(*native->ext));
    if (!native->type || !native->ext)
    {
        return 0;
    }
    Type *entry = dtype(freeheap, 64, NULL, 0);
    if (!entry)
    {
        return 0;
    }
    native->type[native->ntype++] = entry;
    runtime(native, &native->ext[0], "prepare", GATE_SIGNATURE, prepare_call, entry);
    runtime(native, &native->ext[1], "finish", GATE_SIGNATURE, finish_call, entry);
    Import bridge[] = {{GATE_SIGNATURE, "invoke"}, {0, NULL}};
    Import helpers[] = {{GATE_SIGNATURE, "prepare"}, {GATE_SIGNATURE, "finish"}, {0, NULL}};

    build->native = NULL;
    build->helpers = linkmod(native, helpers, 0);
    if (build->helpers == H)
    {
        return 0;
    }
    ptradd(D2H(build->helpers));
    build->helper_root = 1;
    GatewayData *owned = H2D(GatewayData *, heapz(mp));
    module->mptype = 1;
    module->origmp = (uchar *)owned;
    owned->target = H;
    owned->helpers = H;
    if (dvm_refcount_retain_u(&D2H(data->target)->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    Setmark(D2H(data->target));
    Setmark(D2H(build->helpers));
    data->helpers = build->helpers;
    *owned = *data;
    ptrdel(D2H(build->helpers));
    build->helper_root = 0;
    build->helpers = H;
    build->module = NULL;
    build->link = linkmod(module, bridge, 1);
    return build->link != H;
}


/* [GC:] EN: Bind to slot zero of a rooted original link. Return an owned bridge or H.
 * RU: Связать со слотом ноль удерживаемого исходного модуля. Вернуть шлюз либо H. */
Modlink *dvm_styx_managed_gateway(Modlink *target, const StyxCallShape *shape)
{
    size_t words;
    unsigned char references;
    if (!target || target == H || !target->prog || target->nlinks != 1 ||
        !dvm_styx_value_shape_supported(shape->arguments) ||
        !dvm_styx_value_shape_supported(shape->result) ||
        !dvm_styx_frame_layout(shape->arguments, &words, &references) ||
        !dvm_styx_export_frame(target->links[0].frame, words, references))
    {
        return H;
    }
    GatewayBuild build = {.data = {.target = target, .shape = *shape},
        .link = H, .helpers = H};
    if (!kprotect(build_gateway, &build))
    {
        if (build.helper_root)
        {
            ptrdel(D2H(build.helpers));
        }
        destroy(build.helpers);
        if (build.module)
        {
            unload(build.module);
        }
        if (build.native)
        {
            unload(build.native);
        }
        return H;
    }
    return build.link;
}
