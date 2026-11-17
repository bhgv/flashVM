/* Copyright (c) 2026 bhgv
 * EN: Carrier ownership and strict native I/O validation, independent of OS handles and 9P.
 * RU: Carrier ownership и strict native I/O validation независимо от OS handles и 9P.
 */
#include "phy.h"
#include <stdlib.h>


/* EN/RU: Single native I/O owner; only immutable callback/state access is concurrent for wake.
 * Один native I/O owner; только immutable callback/state для concurrent wake. */
struct RemotePhy
{
    PhyOps ops; /* Immutable copied driver entry points. */
    void *state; /* Owned driver state until destroy. */
    PhyInfo info; /* Owner-only local capabilities/lifecycle. */
};


/* EN/RU: Construct an initialized acknowledged zero result; no callbacks/ownership.
 * Создать initialized acknowledged zero result без callbacks/ownership. */
static PhyTransfer result(int status)
{
    PhyTransfer out = {0, status, PHY_PREFIX};
    return out;
}


/* EN/RU: See phy.h; no ownership changes on malformed capabilities/callbacks or OOM.
 * См. phy.h; ownership не меняется при malformed input/OOM. */
RemotePhy *phycreate(const PhyOps *ops, void *state, const PhyInfo *info)
{
    if (!ops || !info || !ops->read || !ops->write || !ops->wait || !ops->wake ||
        !ops->disconnect || !ops->destroy || (info->connected != 0 && info->connected != 1) ||
        (info->capabilities & ~(PHY_STREAM|PHY_ORDERED|PHY_RELIABLE|PHY_HALF_DUPLEX|PHY_PACKET)) ||
        !(info->capabilities & (PHY_STREAM|PHY_PACKET)) ||
        ((info->capabilities & PHY_STREAM) && (info->capabilities & PHY_PACKET)))
    {
        return NULL;
    }
    RemotePhy *phy = malloc(sizeof(*phy));
    if (phy)
    {
        phy->ops = *ops;
        phy->state = state;
        phy->info = *info;
    }
    return phy;
}


/* EN/RU: See phy.h; copied local snapshot, never a readiness or permission probe.
 * См. phy.h; local snapshot, не readiness/permission probe. */
int phyinfo(RemotePhy *phy, PhyInfo *out)
{
    if (!phy || !out)
    {
        return 0;
    }
    *out = phy->info;
    return 1;
}


/* EN/RU: Validate only returned acknowledgement against actual offered capacity; preserve known prefix.
 * Проверить acknowledgement относительно actual capacity; сохранить known prefix. */
static PhyTransfer validate(PhyTransfer out, size_t size, int writing)
{
    if (out.count > size || !(out.flags & PHY_PREFIX) || (out.flags & ~(PHY_PREFIX|PHY_UNCERTAIN)) ||
        (out.status != PHY_OK && out.status != PHY_AGAIN && out.status != PHY_EOF && out.status != PHY_ERROR) ||
        (out.status == PHY_EOF && (writing || out.count)) ||
        (out.status == PHY_OK && !out.count))
    {
        out = result(PHY_ERROR);
        out.flags |= PHY_UNCERTAIN;
    }
    return out;
}


/* EN/RU: See phy.h; reject before driver entry on invalid/disconnected/empty input.
 * См. phy.h; отказ до driver entry при invalid/disconnected, empty input без callback. */
PhyTransfer phyread(RemotePhy *phy, void *bytes, size_t size)
{
    if (!phy || !phy->info.connected || (!bytes && size))
    {
        return result(PHY_ERROR);
    }
    if (!size)
    {
        return result(PHY_OK);
    }
    if (phy->info.max_read && size > phy->info.max_read)
    {
        size = phy->info.max_read;
    }
    return validate(phy->ops.read(phy->state, bytes, size), size, 0);
}


/* EN/RU: See phy.h; do not split/retry an uncertain or partially confirmed operation.
 * См. phy.h; не split/retry uncertain/partial operation. */
PhyTransfer phywrite(RemotePhy *phy, const void *bytes, size_t size)
{
    if (!phy || !phy->info.connected || (!bytes && size))
    {
        return result(PHY_ERROR);
    }
    if (!size)
    {
        return result(PHY_OK);
    }
    if (phy->info.max_write && size > phy->info.max_write)
    {
        size = phy->info.max_write;
    }
    return validate(phy->ops.write(phy->state, bytes, size), size, 1);
}


/* EN/RU: See phy.h; driver wait is outside protocol locks; reject invalid bits without publication.
 * См. phy.h; driver wait вне protocol locks, invalid bits не публикуются. */
int phywait(RemotePhy *phy, unsigned wanted, int timeout, unsigned *ready)
{
    unsigned observed = 0;
    if (!phy || !phy->info.connected || !ready || !wanted || (wanted & ~(PHY_READ|PHY_WRITE)) || timeout < -1 ||
        phy->ops.wait(phy->state, wanted, timeout, &observed) != PHY_OK ||
        (observed & ~(PHY_READ|PHY_WRITE|PHY_WAKE|PHY_HANGUP|PHY_FAILED)))
    {
        return PHY_ERROR;
    }
    *ready = observed;
    return PHY_OK;
}


/* EN/RU: See phy.h; no local lifecycle read races the native owner on this path.
 * См. phy.h; здесь нет local lifecycle read, гоняющегося с native owner. */
void phywake(RemotePhy *phy)
{
    if (phy)
    {
        phy->ops.wake(phy->state);
    }
}


/* EN/RU: See phy.h; output is copied only after successful optional query.
 * См. phy.h; output копируется только после successful optional query. */
int phyavailable(RemotePhy *phy, size_t *bytes)
{
    if (!phy || !bytes || !phy->info.connected)
    {
        return PHY_ERROR;
    }
    size_t count = 0;
    if (!phy->ops.available)
    {
        return PHY_UNSUPPORTED;
    }
    int status = phy->ops.available(phy->state, &count);
    if (status == PHY_OK)
    {
        *bytes = count;
    }
    return status == PHY_OK || status == PHY_UNSUPPORTED ? status : PHY_ERROR;
}


/* EN/RU: See phy.h; no implicit dial/authentication, only the chosen driver's native setup.
 * См. phy.h; без implicit dial/authentication, только native setup выбранного driver. */
int phyconnect(RemotePhy *phy, const void *parameters, size_t size)
{
    if (!phy || (!parameters && size))
    {
        return PHY_ERROR;
    }
    if (phy->info.connected)
    {
        return !parameters && !size ? PHY_OK : PHY_ERROR;
    }
    int status = phy->ops.connect ? phy->ops.connect(phy->state, parameters, size) : PHY_UNSUPPORTED;
    if (status == PHY_OK)
    {
        phy->info.connected = 1;
    }
    return status == PHY_OK || status == PHY_UNSUPPORTED ? status : PHY_ERROR;
}


/* EN/RU: See phy.h; disconnect once, retain driver state for concurrent wake users until final free.
 * См. phy.h; disconnect один раз, driver state для wake остаётся до free. */
void phydisconnect(RemotePhy *phy)
{
    if (phy && phy->info.connected)
    {
        phy->info.connected = 0;
        phy->ops.disconnect(phy->state);
    }
}


/* EN/RU: See phy.h; final free requires all I/O/wake callers to have retired.
 * См. phy.h; final free после всех I/O/wake callers. */
void phyfree(RemotePhy *phy)
{
    if (phy)
    {
        phydisconnect(phy);
        phy->ops.destroy(phy->state);
        free(phy);
    }
}
