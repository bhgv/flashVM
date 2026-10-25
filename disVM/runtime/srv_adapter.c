/* Retained Srv owns all VM results; this adapter supplies only native resolver services. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "network/resolver.h"
#include "channel_sync.h"

static FVM_PortSync *resolver_sync; /* Owned native serialization primitive, retained until process exit. */


/* Initialize real resolver serialization before registering $Srv. Returns nothing;
 * allocation failure raises before any module can call this adapter. */
void dvm_srv_initialize(void)
{
    resolver_sync = fvm_port_sync_create();
    if (!resolver_sync)
    {
        error(exNomem);
    }
}


/* Serialize native lookups; called only from the current standalone VM owner. */
void dvm_srv_lock(void)
{
    fvm_port_sync_lock(resolver_sync);
}


/* Release native lookup serialization, including the production exception path. */
void dvm_srv_unlock(void)
{
    fvm_port_sync_unlock(resolver_sync);
}


/* Reject nil and embedded NUL before converting a borrowed VM lookup argument.
 * Returns borrowed UTF-8 conversion storage; no heap pointer crosses a VM boundary. */
char *dvm_srv_string(String *value)
{
    int index;
    if (value == nil || value == H)
    {
        error(exNilref);
    }
    if (value->len == (-0x7fffffff - 1))
    {
        error(exBounds);
    }
    for (index = 0; index < (value->len < 0 ? -value->len : value->len); index++)
    {
        if ((value->len < 0 ? value->Srune[index] : (uchar)value->Sascii[index]) == 0)
        {
            error("resolver argument contains NUL");
        }
    }
    return string2c(value);
}


/* Copy bounded native answers into production Srv's already-zeroed owner vector.
 * result/hosts are borrowed. count limits writes; allocation errors are raised so
 * the retained Srv cleanup releases all earlier strings and the vector. */
static int srv_answers(const FVM_ResolverNames *result, char **hosts, int capacity)
{
    int index;
    int count = result->count < (size_t)capacity ? (int)result->count : capacity;
    for (index = 0; index < count; index++)
    {
        size_t size = strlen(result->names[index]) + 1;
        hosts[index] = malloc(size);
        if (!hosts[index])
        {
            error(exNomem);
        }
        memcpy(hosts[index], result->names[index], size);
    }
    return count;
}


/* Resolve a borrowed host into at most capacity owned strings for production Srv.
 * Returns the count, zero on a resolver failure; allocation failure raises. */
int so_gethostbyname(char *host, char **hosts, int capacity)
{
    FVM_ResolverNames result;
    if (fvm_port_resolve_host(host, &result) != FVM_OK)
    {
        return 0;
    }
    return srv_answers(&result, hosts, capacity);
}


/* Reverse a borrowed numeric address; output ownership and failures match forward lookup. */
int so_gethostbyaddr(char *address, char **hosts, int capacity)
{
    FVM_ResolverNames result;
    if (fvm_port_resolve_address(address, &result) != FVM_OK)
    {
        return 0;
    }
    return srv_answers(&result, hosts, capacity);
}


/* Resolve borrowed service/network text into production Srv's sixteen-byte port buffer.
 * Returns zero on success, minus one on invalid/unavailable service; buffer then stays untouched. */
int so_getservbyname(char *service, char *network, char *buffer)
{
    unsigned int port;
    if (fvm_port_resolve_service(network, service, &port) != FVM_OK)
    {
        return -1;
    }
    snprint(buffer, 16, "%ud", port);
    return 0;
}
