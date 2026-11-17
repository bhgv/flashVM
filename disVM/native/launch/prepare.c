/* Copyright (c) 2026 bhgv
 * EN: Reuse ordinary rooted Styx mount and atomic launch union, with preflight rejection.
 * RU: Обычный rooted Styx mount и атомарный launch union с preflight rejection.
 */
#include "prepare.h"
#include "mount.h"
#include "cancel.h"
#include <string.h>


/* EN/RU: Reject invalid policy/grants/provenance before any remote metadata request.
 * Borrow options/access, return1/0; no I/O or access authority from identity text.
 * Проверить options/access до remote metadata;1/0, без I/O и authority из identity. */
static int valid(const DVM_LaunchOptions *options, unsigned access)
{
    return options && !options->terminal && options->provider &&
        options->policy == DVM_LAUNCH_TERMINAL_FIRST && !(access & ~FVM_ACCESS_MASK) &&
        (access & (FVM_ACCESS_READ|FVM_ACCESS_EXEC)) == (FVM_ACCESS_READ|FVM_ACCESS_EXEC) &&
        dvm_launch_origin_valid(&options->terminal_origin, DVM_ORIGIN_TERMINAL) &&
        dvm_launch_origin_valid(&options->provider_origin, DVM_ORIGIN_PROVIDER) &&
        !strcmp(options->terminal_origin.session, options->provider_origin.session);
}


/* EN/RU: See prepare.h; temporary root is always released, resulting namespace holds its own pins.
 * См. prepare.h; temporary root всегда освобождается, namespace владеет отдельными pins. */
DVM_Namespace *styxlaunchprepare(StyxClient *terminal, const DVM_LaunchOptions *options, unsigned access)
{
    if (!terminal || !valid(options, access) || dvm_cancel_requested(dvm_cancel_current()))
    {
        styxclientfree(terminal);
        return NULL;
    }
    DVM_Provider *root = styxmountproviderat(terminal, ".", access);
    if (!root)
    {
        return NULL;
    }
    DVM_LaunchOptions prepared = *options;
    prepared.terminal = root;
    DVM_Namespace *group = dvm_launch_prepare(&prepared);
    dvm_provider_release(root);
    return group;
}
