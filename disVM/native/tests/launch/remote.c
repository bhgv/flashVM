/* Copyright (c) 2026 bhgv
 * EN: Real Styx terminal root in private launch union, preflight/cancel checks and per-file reorder.
 * RU: Настоящий Styx terminal root в private launch union, preflight/cancel и per-file reorder.
 */
#include "prepare.h"
#include "../../../runtime/vfs/internal.h"
#include "../../../runtime/vfs/stack.h"
#include "../../../runtime/vfs/cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Caller-owned fixture state remains alive until final local provider release.
 * Fixture state живёт до final local provider release. */
typedef struct Source
{
    const char *name; /* Borrowed overlapping resource name. */
    unsigned fallback; /* Local-only metadata queried only after confirmed absence. */
    unsigned denied; /* Must stay zero after remote search denial. */
    unsigned destroyed; /* Final native destructor count. */
} Source;


/* EN/RU: Fatal fixture invariant in every build profile, diagnostic borrowed. / Явный fatal invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "launch remote root: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Local execution-provider metadata, overlapping terminal resource with independent identity.
 * Local metadata с overlapping terminal name и independent identity. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    Source *source = opaque;
    int root = !strcmp(path, ".");
    if (!strcmp(path, "localonly"))
    {
        source->fallback++;
    }
    if (!strcmp(path, "locked/secret") || !strcmp(path, "statgone"))
    {
        source->denied++;
    }
    if (!root && strcmp(path, source->name) && strcmp(path, "localonly") && strcmp(path, "locked/secret") && strcmp(path, "statgone"))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = root;
    out->identity.low = root ? 101 : 102;
    out->mode = root ? 0555 : 0444;
    strcpy(out->name, root ? "/" : source->name);
    return FVM_OK;
}


/* EN/RU: Read one native local byte, exact borrowed output, no VM calls. / Прочитать один local byte. */
static FVM_Result readbyte(void *state, void *bytes, size_t size, size_t *count)
{
    (void)state;
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = 'L';
    }
    return FVM_OK;
}


/* EN/RU: No inner allocation; outer provider retains Source through open. / Inner allocation отсутствует. */
static void closebyte(void *state)
{
    (void)state;
}
static const DVM_VfsOps stream = {NULL, readbyte, NULL, NULL, NULL, closebyte}; /* Native readonly resource. */


/* EN/RU: Open only the fixture's granted readonly name; output borrows provider-held Source.
 * Открыть только granted readonly name; output заимствует Source под provider pin. */
static FVM_Result openbyte(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo attributes;
    if (strcmp(mode, "r") || info(state, path, &attributes) != FVM_OK || attributes.directory)
    {
        return FVM_ERROR;
    }
    out->ops = &stream;
    out->state = state;
    return FVM_OK;
}


/* EN/RU: Final local factory witness; Source itself remains caller-owned. / Final factory witness. */
static void destroy(void *state)
{
    ((Source *)state)->destroyed++;
}
static const DVM_ProviderOps ops = {info, openbyte, destroy, DVM_PROVIDER_STABLE_BIND}; /* Live local root. */


/* EN/RU: Deterministic canonical associations, trusted fixture metadata rather than credentials.
 * Canonical associations для fixture, не credentials. */
static void origin(DVM_Origin *out, unsigned role, char label)
{
    memset(out, 0, sizeof(*out));
    out->role = role;
    memset(out->system, label, 42);
    out->system[42] = 'A';
    memset(out->export_id, label, 42);
    out->export_id[42] = 'A';
    memset(out->session, 'S', 42);
    out->session[42] = 'A';
}


/* EN/RU: Read a chosen live namespace resource and return copied source role; close every plan/handle.
 * Прочитать live namespace resource, вернуть role; закрыть plan/handle. */
static unsigned readresource(DVM_Namespace *group, const char *path, const char *expected)
{
    DVM_ResourcePlan plan;
    FVM_FileInfo attributes;
    DVM_VfsHandle handle = {0};
    char bytes[32];
    size_t count;
    check(dvm_resource_plan(group, path, 0, &plan), "resource plan");
    check(dvm_resource_info(&plan, &attributes) == FVM_OK, "resource metadata");
    check(dvm_provider_open(plan.provider, plan.relative, "r", &handle) == FVM_OK, "resource open");
    DVM_IoResult read = dvm_vfs_read_chunk(&handle, bytes, sizeof(bytes));
    check(read.status == FVM_OK && (read.flags & DVM_IO_PREFIX) && !(read.flags & DVM_IO_UNCERTAIN), "resource read status");
    count = read.count;
    if (count != strlen(expected) || memcmp(bytes, expected, count))
    {
        fprintf(stderr, "launch bytes path=%s role=%u count=%zu expected=%s\n", path, plan.origin.role, count, expected);
        check(0, "resource payload");
    }
    unsigned role = plan.origin.role;
    dvm_vfs_close(&handle);
    dvm_resource_close(&plan);
    return role;
}


/* EN/RU: Supply an owned extra ref to a refusal; original client must remain healthy.
 * Передать own extra ref для refusal; original client должен остаться healthy. */
static void refused(StyxClient *client, const DVM_LaunchOptions *options, unsigned access)
{
    check(styxclientretain(client) && !styxlaunchprepare(client, options, access) && styxclientalive(client),
        "invalid preparation published environment or consumed caller ref");
}


/* EN/RU: Borrow attached client/name/expected through return; both peers keep ordinary RPCs alive.
 * Borrow client/name/expected до return; обе стороны продолжают обычные RPCs. */
void launchremote(StyxClient *client, const char *name, const char *expected)
{
    Source source = {name, 0, 0, 0};
#ifdef STYX_INFERNO_LOOKUP_TEST
    check(styxclienterrorprofile(client, STYX_ERRORS_INFERNO), "explicit Inferno lookup profile");
#endif
    DVM_LaunchOptions options = {0};
    options.policy = DVM_LAUNCH_TERMINAL_FIRST;
    options.provider = dvm_provider_create_native(&ops, &source, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    check(options.provider != NULL, "local prepared grant");
    origin(&options.terminal_origin, DVM_ORIGIN_TERMINAL, 'T');
    origin(&options.provider_origin, DVM_ORIGIN_PROVIDER, 'P');
    unsigned access = FVM_ACCESS_READ|FVM_ACCESS_EXEC;
    options.policy = 0;
    refused(client, &options, access);
    options.policy = DVM_LAUNCH_TERMINAL_FIRST;
    refused(client, &options, FVM_ACCESS_READ);
    options.terminal = options.provider;
    refused(client, &options, access);
    options.terminal = NULL;
    options.provider_origin.session[0] = 'X';
    refused(client, &options, access);
    options.provider_origin.session[0] = 'S';
    DVM_Cancel *cancel = dvm_cancel_create();
    DVM_CancelScope scope = {0};
    check(cancel && dvm_cancel_enter(&scope, cancel), "native cancel scope");
    dvm_cancel_request(cancel);
    refused(client, &options, access);
    check(dvm_cancel_leave(&scope), "cancel scope retirement");
    dvm_cancel_release(cancel);
    check(styxclientretain(client), "prepared terminal client ref");
    DVM_Namespace *group = styxlaunchprepare(client, &options, access);
    check(group && group->absence_only && group->no_native_fallback && !group->root_anchor && !options.terminal,
        "private launch namespace and immutable input options");
    char path[64];
    snprintf(path, sizeof(path), "/%s", name);
    check(readresource(group, path, expected) == DVM_ORIGIN_TERMINAL, "remote terminal first");
    check(!styxclienterrorprofile(client, STYX_ERRORS_OPAQUE), "profile cannot change after publication");
    DVM_ResourcePlan fallback;
    FVM_FileInfo current;
#ifdef STYX_INFERNO_LOOKUP_TEST
    check(readresource(group, "/localonly", "L") == DVM_ORIGIN_PROVIDER && source.fallback > 0,
        "confirmed remote absence selects local grant");
#else
    check(dvm_resource_plan(group, "/localonly", 0, &fallback) &&
        dvm_resource_info(&fallback, &current) != FVM_OK && !source.fallback,
        "opaque default refuses text-based fallback");
    dvm_resource_close(&fallback);
#endif
    check(dvm_resource_plan(group, "/locked/secret", 0, &fallback) &&
        dvm_resource_info(&fallback, &current) != FVM_OK && !source.denied,
        "remote search denial never tries local provider");
    dvm_resource_close(&fallback);
    check(dvm_resource_plan(group, "/statgone", 0, &fallback) &&
        dvm_resource_info(&fallback, &current) != FVM_OK && !source.denied,
        "successful walk followed by failed stat never selects lower grant");
    dvm_resource_close(&fallback);
    DVM_Namespace *copy = dvm_namespace_copy(group);
    DVM_ResourcePlan plan;
    check(copy && dvm_resource_plan(group, path, 0, &plan) && plan.layers &&
        plan.origin.role == DVM_ORIGIN_TERMINAL && plan.layers->origin.role == DVM_ORIGIN_PROVIDER,
        "both live provider positions");
    check(dvm_stack_move(&plan, plan.names.version, plan.layer_id, -1) == 1, "move only selected file");
    dvm_resource_close(&plan);
    check(readresource(group, path, "L") == DVM_ORIGIN_PROVIDER &&
        readresource(copy, path, expected) == DVM_ORIGIN_TERMINAL, "local reorder leaves copied environment independent");
    check(dvm_resource_plan(copy, path, 0, &plan), "readonly plan");
    DVM_VfsHandle denied = {0};
    check(dvm_provider_open(plan.provider, plan.relative, "wo", &denied) != FVM_OK, "local ceiling denies writes");
    dvm_resource_close(&plan);
    FVM_FileInfo attributes;
    check(dvm_resource_plan(copy, "/etc/passwd", 0, &plan) && dvm_resource_info(&plan, &attributes) != FVM_OK,
        "missing path never escapes to host root");
    dvm_resource_close(&plan);
    dvm_namespace_release(group);
    dvm_namespace_release(copy);
    dvm_provider_release(options.provider);
    check(source.destroyed == 1 && styxclientalive(client), "namespace retirement preserves original client");
}
