/* Copyright (c) 2026 bhgv
 * EN: Decode rooted frozen frame links without executing guest or GC callbacks.
 * RU: Читать rooted frozen frame links без guest либо GC callbacks.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "internal.h"
#include "frames.h"
#include <stddef.h>
#include <string.h>


/* [GC:] EN: Borrow frame's immutable descriptor; nil t belongs to its own extent
 * header. Geometry is checked, native corruption is not made safe by this helper.
 * RU: Заимствовать immutable descriptor frame; nil t принадлежит его extent header.
 * Проверяется geometry; helper не делает native corruption безопасным. */
static Type *frame_type(void *frame)
{
    Type *type;
    uintptr address = (uintptr)frame;
    if (!frame || frame == H || address > (uintptr)-1 - sizeof(Frame))
    {
        return NULL;
    }
    memcpy(&type, (uchar *)frame + offsetof(Frame, t), sizeof(type));
    if (!type)
    {
        size_t header = offsetof(Stkext, reg.tos.fu);
        if (address < header)
        {
            return NULL;
        }
        memcpy(&type, (void *)(address - header + offsetof(Stkext, reg.TR)), sizeof(type));
    }
    if (!type || type == H || type->size < (int)sizeof(Frame) || type->np < 0)
    {
        return NULL;
    }
    return type;
}


/* [GC:] EN/RU: Read bounded frozen chain, skipping the native builtin's non-Dis frame.
 * Читать bounded frozen chain, пропуская non-Dis frame native builtin. */
int dvm_debug_frame(const DebugTarget *target, unsigned int index, DebugFrame *output)
{
    void *fp = target->registers.FP;
    Modlink *module = target->registers.M;
    const void *pc = target->registers.PC;
    unsigned int step = 0;
    if (index >= 128 || !module || module == H || !module->m || module->m == H)
    {
        return 0;
    }
    if (!module->prog || module->prog == H)
    {
        Frame links;
        if (!frame_type(fp))
        {
            return 0;
        }
        memcpy(&links, fp, sizeof(links));
        module = links.mr;
        pc = links.lr;
        fp = links.fp;
    }
    for (;;)
    {
        Frame links;
        Type *type = frame_type(fp);
        if (!type || !module || module == H || !module->m || module->m == H)
        {
            return 0;
        }
        if (step == index)
        {
            output->data = fp;
            output->type = type;
            output->module = module;
            output->pc = pc;
            return 1;
        }
        memcpy(&links, fp, sizeof(links));
        if (!links.fp || links.fp == H || links.fp == fp)
        {
            return 0;
        }
        fp = links.fp;
        pc = links.lr;
        if (links.mr)
        {
            module = links.mr;
        }
        step++;
    }
}
