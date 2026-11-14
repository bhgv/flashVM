/* Copyright (c) 2026 bhgv
 * EN: On-request GC root/graph census, with no permanent per-object statistics.
 * RU: GC root/graph census по запросу, без постоянной статистики по объектам.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "internal.h"
#include "census.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* [GC:] EN: Walk all allocated extent frames just as GC rootset does, including
 * prepared argument/native frames outside the active FP chain. Source is frozen.
 * RU: Читать все allocated extent frames как GC rootset, включая prepared
 * argument/native frames вне active FP chain. Source остаётся frozen. */
static int roots(DebugTarget *target, DebugCensus *census)
{
    unsigned char *extent = target->registers.EX;
    unsigned char *sp = target->registers.SP;
    unsigned char *top = target->registers.TS;
    unsigned int extents = 0;
    if (!dvm_debug_census_field(census, &target->registers.M, 1) ||
        !dvm_debug_census_field(census, &target->process->exval, 1))
    {
        return 0;
    }
    while (extent)
    {
        Stkext *stack = (Stkext *)extent;
        unsigned char *cursor;
        Type *root_type;
        unsigned char *previous_extent;
        unsigned char *previous_sp;
        unsigned char *previous_top;
        if (extent == H || (uintptr)extent > (uintptr)-1 - offsetof(Stkext, reg.tos.fu) || ++extents > 64)
        {
            census->flags |= 2;
            return 0;
        }
        memcpy(&root_type, &stack->reg.TR, sizeof(root_type));
        memcpy(&previous_extent, &stack->reg.EX, sizeof(previous_extent));
        memcpy(&previous_sp, &stack->reg.SP, sizeof(previous_sp));
        memcpy(&previous_top, &stack->reg.TS, sizeof(previous_top));
        cursor = stack->reg.tos.fu;
        if ((uintptr)sp < (uintptr)cursor || (uintptr)top < (uintptr)sp)
        {
            census->flags |= 8;
            return 0;
        }
        while (cursor != sp)
        {
            Frame frame;
            Type *type;
            size_t remaining = (uintptr)sp - (uintptr)cursor;
            if (remaining < sizeof(frame) || census->frames == 128)
            {
                census->flags |= 2;
                return 0;
            }
            memcpy(&frame, cursor, sizeof(frame));
            type = frame.t ? frame.t : root_type;
            if (!type || type == H || type->size < (int)sizeof(frame) || (size_t)type->size > remaining)
            {
                census->flags |= 8;
                return 0;
            }
            census->frames++;
            census->frame_bytes += (unsigned long)type->size;
            if (!dvm_debug_census_field(census, cursor + offsetof(Frame, mr), 1) ||
                !dvm_debug_census_map(census, type, cursor, (size_t)type->size, 1))
            {
                return 0;
            }
            cursor += type->size;
        }
        if (previous_extent == extent)
        {
            census->flags |= 8;
            return 0;
        }
        extent = previous_extent;
        sp = previous_sp;
        top = previous_top;
    }
    return 1;
}


/* [GC:] EN: Collect only on explicit command. Busy/invalid results discard counts;
 * bounded/opaque results keep a marked partial census. Shared objects belong to
 * reachability, never exclusively to one thread; different censuses cannot sum.
 * RU: Собирать только explicit command. Busy/invalid отбрасывают counts;
 * bounded/opaque сохраняют помеченный partial census. Shared objects учитывают
 * reachability, не exclusive thread ownership; разные censuses нельзя складывать. */
void dvm_debug_census(DebugTarget *target, char *output, size_t capacity)
{
    DebugCensus census = {0};
    DVM_CellResult result = dvm_managed_ranges_begin(NULL, 0, NULL, 0, &census.access);
    if (result != DVM_CELL_ACQUIRED)
    {
        census.flags = result == DVM_CELL_BUSY ? 4 : 8;
    }
    else
    {
        if (roots(target, &census))
        {
            dvm_debug_census_graph(&census);
        }
        if (!dvm_range_access_end(&census.access))
        {
            abort();
        }
    }
    if (census.flags & (4 | 8))
    {
        unsigned int flags = census.flags;
        memset(&census, 0, sizeof(census));
        census.flags = flags;
    }
    if (!census.flags)
    {
        census.flags = 1;
    }
    snprintf(
        output,
        capacity,
        "{\"ok\":true,\"flags\":%u,\"roots\":%u,\"fields\":%u,\"objects\":%u,\"bytes\":%lu,\"frames\":%u,\"frame_bytes\":%lu,\"arrays\":[%u,%lu],\"strings\":[%u,%lu],\"lists\":[%u,%lu],\"channels\":[%u,%lu],\"modules\":[%u,%lu],\"records\":[%u,%lu],\"opaque\":[%u,%lu]}\n",
        census.flags,
        census.roots,
        census.fields,
        census.count,
        census.bytes,
        census.frames,
        census.frame_bytes,
        census.kinds[0],
        census.kind_bytes[0],
        census.kinds[1],
        census.kind_bytes[1],
        census.kinds[2],
        census.kind_bytes[2],
        census.kinds[3],
        census.kind_bytes[3],
        census.kinds[4],
        census.kind_bytes[4],
        census.kinds[5],
        census.kind_bytes[5],
        census.kinds[6],
        census.kind_bytes[6]);
}
