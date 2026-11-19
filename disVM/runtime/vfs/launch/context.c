/* Copyright (c) 2026 bhgv
 * EN: Bounded immutable startup snapshot; constructor copies and native refcount lifetime only.
 * RU: Ограниченная неизменяемая startup-копия; constructor copy и native refcount lifetime.
 */
#include "context.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: See context.h; validate before reading payload or allocating private snapshot storage.
 * См. context.h; проверить до чтения payload и allocation private snapshot storage.
 */
DVM_LaunchContext *dvm_launch_context_create(const DVM_LaunchHandoff *handoff)
{
    if (!handoff || !handoff->description.module[0] ||
        !memchr(handoff->description.module, 0, sizeof(handoff->description.module)) ||
        handoff->description.argument_bytes > DVM_LAUNCH_ARGUMENT_LIMIT ||
        (handoff->description.argument_bytes && !handoff->arguments) || handoff->description.typed > 1 ||
        (!!handoff->description.typed != !!handoff->contract))
    {
        return NULL;
    }
    size_t length = handoff->description.argument_bytes; /* Bounded before size addition. */
    DVM_LaunchContext *context = malloc(sizeof(*context) + length);
    if (!context)
    {
        return NULL;
    }
    context->contract = dvm_launch_contract_clone(handoff->contract);
    if (handoff->contract && !context->contract)
    {
        free(context);
        return NULL;
    }
    context->references = 1;
    context->argument_bytes = length;
    memset(context->module, 0, sizeof(context->module));
    strcpy(context->module, handoff->description.module);
    if (length)
    {
        memcpy(context->arguments, handoff->arguments, length);
    }
    return context;
}


/* EN/RU: See context.h; caller already owns/pins live source, so zero is never resurrected.
 * См. context.h; caller удерживает live source, нулевой refcount не восстанавливается.
 */
int dvm_launch_context_retain(DVM_LaunchContext *context)
{
    if (!context)
    {
        return 0;
    }
    unsigned count = __atomic_load_n(&context->references, __ATOMIC_RELAXED);
    while (count && count != UINT_MAX)
    {
        if (__atomic_compare_exchange_n(&context->references, &count, count + 1, 0,
                __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See context.h; no callbacks/provider/managed destructors on the final native free.
 * См. context.h; final native free не вызывает callbacks/provider/managed destructors.
 */
void dvm_launch_context_release(DVM_LaunchContext *context)
{
    if (context && __atomic_sub_fetch(&context->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        dvm_launch_contract_free(context->contract);
        free(context);
    }
}
