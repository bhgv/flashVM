/* Copyright (c) 2026 bhgv
 * [GC:] EN: One-shot managed capability publication/root failures before ordinary recovery.
 * RU: One-shot failures публикации/root capability до ordinary recovery.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "vfs/cap_gc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static int refused; /* Atomic one-shot per private runtime process. */
#if CONTROL == 1
const char *__real_dvm_runtime_movp_copy(const void *, void *, void **);


/* [GC:] EN/RU: Refuse only the first exact capability result replacement, preserve old output.
 * Отказать только первой exact capability publication, сохранив прежний output. */
const char *__wrap_dvm_runtime_movp_copy(const void *source, void *destination, void **previous)
{
    void *value;
    memcpy(&value, source, sizeof(value));
    if (value && value != H && dvm_cap_gc_eligible(D2H(value)) && !__atomic_exchange_n(&refused, 1, __ATOMIC_RELAXED))
    {
        return "VFS capability publication refusal";
    }
    return __real_dvm_runtime_movp_copy(source, destination, previous);
}
#elif CONTROL == 2
void __real_ptradd(Heap *heap);


/* [GC:] EN/RU: Refuse root registration before commit; later cleanup uses the real root service.
 * Отказать root registration до commit; cleanup использует настоящий root service. */
void __wrap_ptradd(Heap *heap)
{
    if (dvm_cap_gc_eligible(heap) && !__atomic_exchange_n(&refused, 1, __ATOMIC_RELAXED))
    {
        error("VFS capability publication refusal");
    }
    __real_ptradd(heap);
}
#endif

void __real_dvm_close_worker_stop(void);


/* EN/RU: After the actual closer joins, every constructed native state must be retired.
 * После actual closer join все constructed native states должны быть retired. */
void __wrap_dvm_close_worker_stop(void)
{
    unsigned long created, retired;
    __real_dvm_close_worker_stop();
    dvm_cap_lifetime_snapshot(&created, &retired);
    if (!created || created != retired)
    {
        fprintf(stderr, "VFS capability leak created=%lu retired=%lu\n", created, retired);
        abort();
    }
    puts("PASS VFS capability native lifetimes drained");
}
