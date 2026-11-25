/* Copyright (c) 2026 bhgv
 * EN: Native Unibus capability ownership, explicit inheritance and one-job admission.
 * RU: Владение native Unibus-capability, явное наследование и допуск одного задания.
 */
#include "disvm/unibus_binding.h"
#include "channel_sync.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Native-only state, protected by a portable per-binding mutex; no managed roots.
 * Только native-состояние под переносимым mutex одной привязки; managed roots отсутствуют.
 */
struct UnibusBinding
{
    FVM_PortSync *lock; /* Protects references, admission and revocation; never held across callbacks. */
    unsigned long references; /* Native strong references, never resurrected. */
    uintptr_t owner; /* Opaque identity assigned before PID/publication; never dereferenced. */
    int owner_alive; /* Prevents native identity/address reuse from recovering owner-only rights. */
    int retired; /* Sticky revocation, under lock. */
    unsigned flags; /* Immutable explicit inheritance policy. */
    UnibusOps ops; /* Immutable callbacks, copied before publication. */
    void *state; /* Native state owned according to ops.destroy. */
};


/* EN/RU: See unibus_binding.h; input remains caller-owned on validation/allocation/lock failure.
 * См. unibus_binding.h; при отказе state остаётся у caller.
 */
UnibusBinding *unibusbinding(const UnibusOps *ops, void *state, unsigned flags)
{
    if (!ops || !ops->invoke || (flags & ~(UNIBUS_INHERIT | UNIBUS_OWNER_PORTS | UNIBUS_CLOSE_WITH_OWNER)))
    {
        return NULL;
    }
    UnibusBinding *binding = calloc(1, sizeof(*binding)); /* Unpublished native owner. */
    if (!binding)
    {
        return NULL;
    }
    binding->lock = fvm_port_sync_create();
    if (!binding->lock)
    {
        free(binding);
        return NULL;
    }
    binding->references = 1;
    binding->flags = flags;
    binding->ops = *ops;
    binding->state = state;
    return binding;
}


/* EN/RU: See unibus_binding.h; caller independently pins storage through this operation.
 * См. unibus_binding.h; caller отдельно удерживает память на время операции.
 */
int unibusretain(UnibusBinding *binding)
{
    if (!binding)
    {
        return 0;
    }
    fvm_port_sync_lock(binding->lock);
    int valid = binding->references && binding->references != ULONG_MAX; /* Refuse overflow/resurrection. */
    if (valid)
    {
        binding->references++;
    }
    fvm_port_sync_unlock(binding->lock);
    return valid;
}


/* EN/RU: See unibus_binding.h; final owner alone disposes state outside the native binding lock.
 * См. unibus_binding.h; последний owner освобождает state без lock привязки.
 */
void unibusrelease(UnibusBinding *binding)
{
    if (!binding)
    {
        return;
    }
    fvm_port_sync_lock(binding->lock);
    if (!binding->references)
    {
        fvm_port_sync_unlock(binding->lock);
        fputs("Unibus binding reference underflow\n", stderr);
        abort();
    }
    int last = --binding->references == 0; /* Only this owner may access storage after a final drop. */
    int close = last && !binding->retired; /* Last inherited ref ends the service if not explicitly revoked. */
    if (close)
    {
        binding->retired = 1;
    }
    fvm_port_sync_unlock(binding->lock);
    if (last)
    {
        if (close && binding->ops.retire)
        {
            binding->ops.retire(binding->state);
        }
        if (binding->ops.destroy)
        {
            binding->ops.destroy(binding->state);
        }
        fvm_port_sync_destroy(binding->lock);
        free(binding);
    }
}


/* EN/RU: See unibus_binding.h; one native root identity may claim the binding before publication.
 * См. unibus_binding.h; до publication привязку занимает одна native root identity.
 */
int unibusclaim(UnibusBinding *binding, uintptr_t owner)
{
    if (!binding || !owner)
    {
        return 0;
    }
    fvm_port_sync_lock(binding->lock);
    int valid = !binding->owner && !binding->retired; /* Claim is one-use even if PIDs are later reused. */
    if (valid)
    {
        binding->owner = owner;
        binding->owner_alive = 1;
    }
    fvm_port_sync_unlock(binding->lock);
    return valid;
}


/* EN/RU: See unibus_binding.h; authorize and retain atomically while parent pins source storage.
 * См. unibus_binding.h; разрешить и удержать под одним lock при pin родителя.
 */
UnibusBinding *unibusinherit(UnibusBinding *binding)
{
    if (!binding)
    {
        return NULL;
    }
    fvm_port_sync_lock(binding->lock);
    int valid = (binding->flags & UNIBUS_INHERIT) && !binding->retired &&
        binding->references && binding->references != ULONG_MAX; /* Private child admission. */
    if (valid)
    {
        binding->references++;
    }
    fvm_port_sync_unlock(binding->lock);
    return valid ? binding : NULL;
}


/* EN/RU: See unibus_binding.h; callbacks run outside the lock on the active guest owner.
 * См. unibus_binding.h; callbacks выполняются без lock на active guest owner.
 */
int unibusinvoke(UnibusBinding *binding, uintptr_t caller, int pid, unsigned method, void *frame)
{
    if (!binding || !caller || pid <= 0 || !frame || method >= UNIBUS_METHODS)
    {
        return 0;
    }
    fvm_port_sync_lock(binding->lock);
    int status = 1; /* Admitted invocation unless retired/unclaimed/foreign-root. */
    if (binding->retired)
    {
        status = -2;
    }
    else if (!binding->owner || ((binding->flags & UNIBUS_OWNER_PORTS) &&
        (method == UNIBUS_ATTACH || method == UNIBUS_DETACH ||
         (method >= UNIBUS_RPCOPEN && method <= UNIBUS_RPCCLOSE)) &&
        (!binding->owner_alive || caller != binding->owner)))
    {
        status = -1;
    }
    fvm_port_sync_unlock(binding->lock);
    if (status != 1)
    {
        return status;
    }
    return binding->ops.invoke(binding->state, pid, method, frame) ? 1 : -3;
}


/* EN/RU: See unibus_binding.h; revoke under lock, call native cancellation once outside it.
 * Already admitted callbacks retain their caller's strong ref and may overlap this cancellation.
 * См. unibus_binding.h; отзыв под lock, отмена один раз вне него. Уже допущенные callbacks
 * удерживаются refs своих callers и могут завершаться одновременно с отменой.
 */
void unibusretire(UnibusBinding *binding, uintptr_t caller)
{
    if (!binding || !caller)
    {
        return;
    }
    fvm_port_sync_lock(binding->lock);
    int changed = binding->owner_alive && binding->owner == caller && !binding->retired; /* Only the live root revokes descendants. */
    if (changed)
    {
        binding->retired = 1;
    }
    fvm_port_sync_unlock(binding->lock);
    if (changed && binding->ops.retire)
    {
        binding->ops.retire(binding->state);
    }
}


/* EN/RU: See unibus_binding.h; keep shared service lifetime independent from the initial thread.
 * См. unibus_binding.h; жизнь общего сервиса не привязана к первоначальному потоку по умолчанию.
 */
void unibusownerretired(UnibusBinding *binding, uintptr_t caller)
{
    if (!binding || !caller)
    {
        return;
    }
    fvm_port_sync_lock(binding->lock);
    int owner = binding->owner == caller && binding->owner_alive; /* Once-only owner retirement. */
    if (owner)
    {
        binding->owner_alive = 0;
    }
    int close = owner && (binding->flags & UNIBUS_CLOSE_WITH_OWNER) && !binding->retired;
    if (close)
    {
        binding->retired = 1;
    }
    fvm_port_sync_unlock(binding->lock);
    if (close && binding->ops.retire)
    {
        binding->ops.retire(binding->state);
    }
}
