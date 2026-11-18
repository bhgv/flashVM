/* Copyright (c) 2026 bhgv
 * EN: Client-local TX wraps explicit directional ancestor grants, never exposes transport tags to clients.
 * RU: Client-local TX оборачивает явные directional ancestor grants, не выдавая transport tags клиентам.
 */
#include "internal.h"


/* EN/RU: Encode one complete record in disjoint scratch and enforce this broker's body limit, 1/0.
 * Inputs/output borrowed through return, no native transport or mapping mutation.
 * Кодировать complete record в disjoint scratch с body limit broker, 1/0, без transport/mapping mutation.
 */
static int encode(ScopeHub *hub, unsigned long tag, const void *bytes, size_t size, void *output, size_t *used)
{
    FVM_SRecord view; /* Borrowed validation-only view, no ownership transfer. */
    size_t verified; /* Exact checked frame size. */
    return fvm_srecord_write(tag, bytes, size, output, hub->limit + 32, used) == FVM_S_OK &&
        fvm_srecord_read(output, *used, hub->limit, &view, &verified) == FVM_S_OK && verified == *used;
}


/* EN/RU: See scope.h; freeze all outgoing ancestors before acceptance so later remapping cannot rewrite TX.
 * См. scope.h; захватить все outgoing ancestors до acceptance, later remapping не переписывает TX.
 */
int styxscopeclientsend(StyxScopeClient *client, unsigned long tag, const void *bytes, size_t size)
{
    if (!client || tag > 0xffffffffUL || (size && !bytes))
    {
        return 0;
    }
    ScopeHub *hub = client->scope->hub; /* Host pins entire root through every client call. */
    unsigned char *first = malloc(hub->limit + 32); /* Disjoint complete-frame scratch. */
    unsigned char *second = malloc(hub->limit + 32); /* Next ancestor wrapper scratch. */
    if (!first || !second)
    {
        free(first);
        free(second);
        return 0;
    }
    size_t used = 0; /* Exact current complete frame size. */
    int valid = encode(hub, tag, bytes, size, first, &used); /* Local tag0 remains inside its own envelope. */
    pthread_mutex_lock(&hub->lock);
    valid = valid && !hub->reason;
    for (StyxScope *scope = client->scope; valid && scope->parent; scope = scope->parent)
    {
        valid = scope->tags[STYX_SCOPE_TX] && encode(hub, scope->tags[STYX_SCOPE_TX], first, used, second, &used);
        unsigned char *swap = first; /* Swap disjoint frame ownership, never overlap codec input/output. */
        first = second;
        second = swap;
    }
    if (valid)
    {
        FVM_SRecord outer; /* The host transport adds exactly the outer framing, never an extra wrapper. */
        size_t checked; /* Full outer frame extent. */
        valid = fvm_srecord_read(first, used, hub->limit, &outer, &checked) == FVM_S_OK && checked == used &&
            outer.tag && styxbidircontrolsend(hub->control, outer.tag, outer.payload, outer.size);
    }
    pthread_mutex_unlock(&hub->lock);
    free(first);
    free(second);
    return valid;
}
