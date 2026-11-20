/* Copyright (c) 2026 bhgv
 * EN: Private immutable destination ownership and shared side-effect-free request preflight.
 * RU: Private immutable destination ownership и общий request preflight без side effects.
 */
#ifndef DVM_LAUNCH_ADDRESS_INTERNAL_H
#define DVM_LAUNCH_ADDRESS_INTERNAL_H
#include "route.h"


/* EN/RU: Address owns only native submit capability and copied owner label, never namespace/VM/root authority.
 * Address владеет лишь native submit capability и copied owner label, без namespace/VM/root authority.
 */
struct DVM_LaunchAddress
{
    void (*dispose)(DVM_LaunchAddress *); /* Optional leaf-native capability cleanup; avoids service-library dependency in guard core. */
    DVM_LaunchAddress *inner; /* Exclusive wrapped address, NULL for leaf; guard chains are freed iteratively. */
    DVM_LaunchCheck check; /* Optional bounded local policy for a wrapper, no provider/guest I/O. */
    void *context; /* Owned guard state only for wrappers; may outlive original provider factory. */
    void (*release)(void *); /* Optional nonblocking native guard cleanup, outside all source/service locks. */
    DVM_LaunchSendProcess process_send; /* Scoped leaf requires a captured caller PID. */
    DVM_LaunchSend send; /* Optional leaf transport, separate from native ticket issuance. */
    DVM_LaunchEndpoint *endpoint; /* Independent owned submit-only wrapper through joined address retirement. */
    char module[DVM_NS_PATH_LIMIT]; /* Immutable owner-side logical label, never copied from request at submit. */
};


/* EN/RU: Validate borrowed caller inputs without destination fields, I/O, allocation or authority acquisition;1/0.
 * Проверить borrowed caller inputs без destination fields, I/O, allocation и acquisition authority;1/0.
 */
int dvm_launch_route_input(const DVM_LaunchSpec *spec);
#endif
