/* Copyright (c) 2026 bhgv
 * EN: Native directory references; last release closes outside namespace locks.
 * RU: Native directory references; последний release закрывает вне namespace mutex.
 */
#include "resource.h"
#include "channel_sync.h"
#include <stdlib.h>


/* EN/RU: Reference mutex protects count only, never child I/O.
 * Reference mutex защищает только count, не дочерний I/O. */
struct DVM_Anchor
{
    FVM_PortAnchor *port; /* Owned immutable directory capability. */
    FVM_PortSync *lock;   /* Reference-count mutex, never an execution gate. */
    size_t references;    /* Owning groups, bindings and in-flight plans. */
};


/* EN/RU: See resource.h; allocate all native wrapper state before opening.
 * См. resource.h; выделить native wrapper до открытия. */
DVM_Anchor *dvm_anchor_open(DVM_Anchor *parent, const char *path)
{
    DVM_Anchor *anchor = calloc(1, sizeof(*anchor));
    FVM_Result status;
    if (!anchor)
    {
        return NULL;
    }
    anchor->lock = fvm_port_sync_create();
    if (!anchor->lock)
    {
        free(anchor);
        return NULL;
    }
    status =
        parent ? fvm_port_anchor_open_at(parent->port, path, &anchor->port) : fvm_port_anchor_open(path, &anchor->port);
    if (status != FVM_OK)
    {
        fvm_port_sync_destroy(anchor->lock);
        free(anchor);
        return NULL;
    }
    anchor->references = 1;
    return anchor;
}


/* EN/RU: See resource.h; caller already protects anchor lifetime.
 * См. resource.h; caller уже защищает lifetime anchor. */
int dvm_anchor_retain(DVM_Anchor *anchor)
{
    int result = 0;
    if (!anchor)
    {
        return 1;
    }
    fvm_port_sync_lock(anchor->lock);
    if (anchor->references < (size_t)-1)
    {
        anchor->references++;
        result = 1;
    }
    fvm_port_sync_unlock(anchor->lock);
    return result;
}


/* EN/RU: See resource.h; destroy only after removing the final owned reference.
 * См. resource.h; уничтожать только после снятия последней ссылки. */
void dvm_anchor_release(DVM_Anchor *anchor)
{
    int last;
    if (!anchor)
    {
        return;
    }
    fvm_port_sync_lock(anchor->lock);
    last = --anchor->references == 0;
    fvm_port_sync_unlock(anchor->lock);
    if (last)
    {
        fvm_port_anchor_close(anchor->port);
        fvm_port_sync_destroy(anchor->lock);
        free(anchor);
    }
}


/* EN/RU: See resource.h; caller retains lifetime throughout use.
 * См. resource.h; caller удерживает lifetime до конца использования. */
FVM_PortAnchor *dvm_anchor_port(DVM_Anchor *anchor)
{
    return anchor ? anchor->port : NULL;
}
