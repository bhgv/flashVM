/* Copyright (c) 2026 bhgv
 * EN: Walk only the active owner's rooted Dis frames into bounded native copies.
 * RU: Читать только rooted Dis frames active owner в ограниченные native copies.
 */
#include "lib9.h"
#include "interp.h"
#include "stack.h"
#include <string.h>


/* [GC:] EN: Borrow live owner roots, copy top plus caller return positions. A nil
 * saved module means a local call; a nonnil saved module crosses an MCALL. Frame
 * headers are copied with memcpy because source frame alignment is not assumed.
 * output/top are disjoint native storage. No references escape or are acquired.
 * RU: Заимствовать живые roots owner, копировать top и caller return positions.
 * Nil saved module означает local call; nonnil — переход MCALL. Копировать headers
 * через memcpy без предположения о frame alignment. output/top — отдельная native
 * память. References не приобретаются и не выходят из функции. */
void dvm_diagnostic_stack_capture(DVM_DiagnosticStack *output,
                                  const DVM_DiagnosticLocation *top,
                                  Modlink *module,
                                  const void *fp)
{
    const void *seen[DVM_DIAG_STACK_FRAMES]; /* Owner-only borrowed addresses, never published. */
    unsigned int visited = 0;
    memset(output, 0, sizeof(*output));
    output->flags = DVM_STACK_UNAVAILABLE;
    if (!(top->flags & DVM_LOCATION_VALID))
    {
        return;
    }
    output->frames[0] = *top;
    output->count = 1;
    for (;;)
    {
        Frame links;
        unsigned int index;
        if (!fp || fp == H || (uintptr)fp > (uintptr)-1 - sizeof(links))
        {
            return;
        }
        for (index = 0; index < visited; index++)
        {
            if (seen[index] == fp)
            {
                return;
            }
        }
        seen[visited++] = fp;
        memcpy(&links, fp, sizeof(links));
        if (!links.fp)
        {
            output->flags = DVM_STACK_COMPLETE;
            return;
        }
        if (links.fp == H || !links.lr || links.lr == H || links.mr == H)
        {
            return;
        }
        if (output->count == DVM_DIAG_STACK_FRAMES)
        {
            output->flags = DVM_STACK_TRUNCATED;
            return;
        }
        if (links.mr)
        {
            module = links.mr;
        }
        dvm_diagnostic_location_copy(&output->frames[output->count], module, links.lr);
        if (!(output->frames[output->count].flags & DVM_LOCATION_VALID))
        {
            memset(&output->frames[output->count], 0, sizeof(output->frames[0]));
            return;
        }
        output->count++;
        fp = links.fp;
    }
}
