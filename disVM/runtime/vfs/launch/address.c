/* Copyright (c) 2026 bhgv
 * EN: Capability-based submit destination; caller cannot replace the owner's module label or service association.
 * RU: Submit destination через capability; caller не заменяет module label владельца и service association.
 */
#include "address_internal.h"
#include "service_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See address_internal.h; destination metadata will be supplied by a real native endpoint instead of peer strings.
 * См. address_internal.h; destination metadata задаёт real native endpoint, не peer strings.
 */
int dvm_launch_route_input(const DVM_LaunchSpec *spec)
{
    return spec && (spec->policy == DVM_LAUNCH_TERMINAL_FIRST || spec->policy == DVM_LAUNCH_OWNER_FIRST) && dvm_launch_root_cancellable(spec->terminal) &&
        dvm_launch_origin_valid(&spec->terminal_origin, DVM_ORIGIN_TERMINAL) &&
        spec->argument_bytes <= DVM_LAUNCH_ARGUMENT_LIMIT && (!spec->argument_bytes || spec->arguments) &&
        (!spec->flashvm || dvm_terminal_channels_valid(spec->flashvm));
}


/* EN/RU: Leaf-only cleanup consumes its owned endpoint, no service close; generic guard core frees storage afterward.
 * Leaf-only cleanup поглощает owned endpoint без service close; generic guard core затем освобождает storage.
 */
static void dispose(DVM_LaunchAddress *address)
{
    dvm_launch_endpoint_free(address->endpoint);
}


/* EN/RU: See route.h; validate the bounded label before allocation and keep caller endpoint ownership unchanged.
 * См. route.h; проверить bounded label до allocation, не менять endpoint ownership caller.
 */
DVM_LaunchAddress *dvm_launch_address_create(DVM_LaunchEndpoint *endpoint, const char *module)
{
    if (!endpoint || !module)
    {
        return NULL;
    }
    size_t length = 0; /* Read only through first NUL or explicit maximum extent. */
    while (length < DVM_NS_PATH_LIMIT && module[length])
    {
        length++;
    }
    if (!length || length == DVM_NS_PATH_LIMIT)
    {
        return NULL;
    }
    DVM_LaunchAddress *address = calloc(1, sizeof(*address));
    if (!address)
    {
        return NULL;
    }
    address->endpoint = dvm_launch_endpoint_clone(endpoint);
    if (!address->endpoint)
    {
        free(address);
        return NULL;
    }
    address->dispose = dispose;
    memcpy(address->module, module, length + 1);
    return address;
}


/* EN/RU: See route.h; endpoint's immutable service identity supplies destination association without granting owner authority.
 * См. route.h; immutable service identity endpoint задаёт destination association, не owner authority.
 */
DVM_LaunchTicket *dvm_launch_address_submit(DVM_LaunchAddress *address, const DVM_LaunchSpec *spec)
{
    if (!address || !dvm_launch_route_input(spec) || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    while (address->inner)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) || address->check(address->context) != 1)
        {
            return NULL;
        }
        address = address->inner;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    if (!address->endpoint)
    {
        return NULL;
    }
    DVM_LaunchSpec routed = *spec; /* Borrowed native grants/argument bytes only during the ordinary copied submit. */
    memcpy(routed.expected_provider, address->endpoint->shared->system, sizeof(routed.expected_provider));
    memcpy(routed.module, address->module, sizeof(routed.module));
    return dvm_launch_service_submit(address->endpoint, &routed);
}
