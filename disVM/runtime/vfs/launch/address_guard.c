/* Copyright (c) 2026 bhgv
 * EN: Generic native address-policy ownership, independent of service queues and VM linkage.
 * RU: Общий native address-policy ownership, независимо от service queues и VM linkage.
 */
#include "address_internal.h"
#include <stdlib.h>


/* EN/RU: See route.h; no callbacks before successful wrapper publication, failed allocation still consumes inner.
 * См. route.h; callbacks не вызываются до success, failed allocation также поглощает inner.
 */
DVM_LaunchAddress *dvm_launch_address_guard(DVM_LaunchAddress *inner, DVM_LaunchCheck check,
    void *context, void (*release)(void *))
{
    if (!inner || !check)
    {
        dvm_launch_address_free(inner);
        return NULL;
    }
    DVM_LaunchAddress *address = calloc(1, sizeof(*address));
    if (!address)
    {
        dvm_launch_address_free(inner);
        return NULL;
    }
    address->inner = inner;
    address->check = check;
    address->context = context;
    address->release = release;
    return address;
}


/* EN/RU: See route.h; endpoint release neither closes its service nor cancels already acknowledged requests.
 * См. route.h; release endpoint не закрывает service и не отменяет acknowledged requests.
 */
void dvm_launch_address_free(DVM_LaunchAddress *address)
{
    while (address)
    {
        DVM_LaunchAddress *inner = address->inner; /* No recursion even for deeply nested native policy wrappers. */
        if (address->release)
        {
            address->release(address->context);
        }
        if (address->dispose)
        {
            address->dispose(address);
        }
        free(address);
        address = inner;
    }
}
