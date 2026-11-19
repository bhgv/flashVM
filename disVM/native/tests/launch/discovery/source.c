/* Copyright (c) 2026 bhgv
 * EN: Native module-address advertisement with correctly rebased captured roots; never reads code images.
 * RU: Native module-address advertisement с корректным rebasing captured roots; без чтения code images.
 */
#include "source.h"
#include "../../../../runtime/vfs/view_internal.h"


/* EN/RU: Factory-local state; independent endpoint pin permits caller's wrapper retirement.
 * Factory-local state; independent endpoint pin допускает retirement wrapper caller.
 */
typedef struct Source
{
    LaunchState data; /* Simple ordinary file metadata/stream fixture. */
    DVM_LaunchEndpoint *endpoint; /* Owned independent submit-only capability. */
    SourceStats *stats; /* Borrowed counters through final factory destruction. */
    unsigned flags; /* Immutable explicit native control-operation promises. */
    int advertise; /* Immutable callback presence, independent of metadata/file suffix. */
    char prefix[DVM_NS_PATH_LIMIT]; /* Copied captured-root identity, never an ambient host path. */
} Source;


/* EN/RU: Join canonical prefix/relative into borrowed bounded output;1/0, no lookup or ownership change.
 * Соединить canonical prefix/relative в bounded output;1/0, без lookup/ownership change.
 */
static int path(Source *source, const char *relative, char *output)
{
    if (!dvm_view_path(relative, 0))
    {
        return 0;
    }
    size_t base = !strcmp(source->prefix, ".") ? 0 : strlen(source->prefix);
    size_t tail = !strcmp(relative, ".") ? 0 : strlen(relative);
    if (base + tail + 2 > DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    memcpy(output, source->prefix, base);
    if (base && tail)
    {
        output[base++] = '/';
    }
    memcpy(output + base, relative, tail);
    output[base + tail] = 0;
    if (!base && !tail)
    {
        strcpy(output, ".");
    }
    return 1;
}


/* EN/RU: Metadata from the exact captured root, using current native cancellation scope.
 * Metadata exact captured root с current native cancellation scope; state/name/output borrowed.
 */
static FVM_Result info(void *state, const char *relative, FVM_FileInfo *output)
{
    Source *source = state;
    char full[DVM_NS_PATH_LIMIT];
    __atomic_add_fetch(&source->stats->metadata, 1, __ATOMIC_RELAXED);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    return path(source, relative, full) ? launch_info(&source->data, full, output) : FVM_NOT_FOUND;
}


/* EN/RU: Ordinary independent stream, no launch delegation merely from opening a readable node.
 * Обычный independent stream; readable open не передаёт launch delegation.
 */
static FVM_Result open_source(void *state, const char *relative, const char *mode, DVM_VfsHandle *output)
{
    Source *source = state;
    char full[DVM_NS_PATH_LIMIT];
    __atomic_add_fetch(&source->stats->opened, 1, __ATOMIC_RELAXED);
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    return path(source, relative, full) ? launch_open(&source->data, full, mode, output) : FVM_NOT_FOUND;
}


/* EN/RU: Advertise only the fixture's explicit module node after root rebasing; owned address/NULL, no image read.
 * Объявить лишь explicit module node после root rebasing; owned address/NULL, без image read.
 */
static DVM_LaunchAddress *launch(void *state, const char *relative)
{
    Source *source = state;
    char full[DVM_NS_PATH_LIMIT];
    __atomic_add_fetch(&source->stats->queries, 1, __ATOMIC_RELAXED);
    if (!path(source, relative, full) || strcmp(full, "dev/keyboard") || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    DVM_LaunchAddress *address = dvm_launch_address_create(source->endpoint, "owned/dev/keyboard");
    if (address && source->stats->cancel_on_query)
    {
        dvm_cancel_request(source->stats->cancel_on_query);
    }
    return address;
}


/* EN/RU: Capture a fresh scoped factory with an explicitly re-established callback, not a blind parent callback copy.
 * Захватить fresh scoped factory с явным callback, не blind copy callback родителя.
 */
static DVM_Provider *capture(void *state, const char *relative)
{
    Source *source = state;
    char full[DVM_NS_PATH_LIMIT];
    FVM_FileInfo metadata;
    if (!path(source, relative, full) || launch_info(&source->data, full, &metadata) != FVM_OK ||
        dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    return source_create(source->endpoint, source->stats, full, source->flags, source->advertise);
}


/* EN/RU: Consume only this factory's native endpoint/state, no service close or target execution.
 * Поглотить лишь native endpoint/state factory, без service close/target execution.
 */
static void destroy(void *state)
{
    Source *source = state;
    __atomic_add_fetch(&source->stats->destroyed, 1, __ATOMIC_RELAXED);
    dvm_launch_endpoint_free(source->endpoint);
    free(source);
}
static const DVM_ProviderOps operations = {info, open_source, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Native file metadata stays separate from launch authority. */


/* EN/RU: See source.h; failed constructor never adopts source storage or the caller's endpoint.
 * См. source.h; failed constructor не принимает source storage и endpoint caller.
 */
DVM_Provider *source_create(DVM_LaunchEndpoint *endpoint, SourceStats *stats, const char *prefix,
    unsigned flags, int advertise)
{
    Source *source = calloc(1, sizeof(*source));
    if (!source || !dvm_view_path(prefix, 0) || !(source->endpoint = dvm_launch_endpoint_clone(endpoint)))
    {
        free(source);
        return NULL;
    }
    source->stats = stats;
    source->flags = flags;
    source->advertise = advertise;
    strcpy(source->prefix, prefix);
    DVM_Provider *provider = dvm_provider_create_routed(&operations, source, capture, 1, flags, advertise ? launch : NULL);
    if (!provider)
    {
        dvm_launch_endpoint_free(source->endpoint);
        free(source);
        return NULL;
    }
    __atomic_add_fetch(&stats->created, 1, __ATOMIC_RELAXED);
    return provider;
}


/* EN/RU: See source.h; policy allows visibility/reading independently from the current execute ceiling.
 * См. source.h; visibility/reading разрешены независимо от current execute ceiling.
 */
int policy_check(void *state, const char *path, unsigned access)
{
    Policy *policy = state;
    (void)path;
    return !(access & FVM_ACCESS_EXEC) || __atomic_load_n(&policy->allowed, __ATOMIC_ACQUIRE);
}


/* EN/RU: See source.h; final context callback owns no factory or service pointer.
 * См. source.h; final context callback не владеет factory/service pointer.
 */
void policy_release(void *state)
{
    Policy *policy = state;
    __atomic_add_fetch(&policy->released, 1, __ATOMIC_RELAXED);
}


/* EN/RU: See source.h; queue inspection is authoritative, not a fabricated call result.
 * См. source.h; queue inspection authoritative, не fabricated call result.
 */
void source_received(DVM_LaunchService *owner, DVM_LaunchTicket *ticket)
{
    DVM_LaunchReview *review = dvm_launch_service_next(owner);
    DVM_LaunchDescription description;
    launch_check(ticket && review && dvm_launch_describe(dvm_launch_review_request(review), &description) &&
        !strcmp(description.module, "owned/dev/keyboard"), "discovered address lost owning source");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(ticket);
}
