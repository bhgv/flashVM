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
    DVM_LaunchEndpoint *endpoint; /* Independent owned submit-only wrapper through joined address retirement. */
    char module[DVM_NS_PATH_LIMIT]; /* Immutable owner-side logical label, never copied from request at submit. */
};


/* EN/RU: Validate borrowed caller inputs without destination fields, I/O, allocation or authority acquisition;1/0.
 * Проверить borrowed caller inputs без destination fields, I/O, allocation и acquisition authority;1/0.
 */
int dvm_launch_route_input(const DVM_LaunchSpec *spec);
#endif
