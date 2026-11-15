/* Copyright (c) 2026 bhgv
 * EN: Prepared ordinary trees, authenticated request interception, ACLs and scanner-provider substitution.
 * RU: Prepared ordinary trees, authenticated request interception, ACLs и scanner-provider substitution.
 */
#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Directory visibility and native lifetime acceptance in separate fixtures.
 * Проверки directory visibility и native lifetime в отдельных fixtures. */
void test_export_hidden_batches(void);
void test_export_races(void);


/* EN/RU: Native policy/filter state, valid through all factory/FD retirement.
 * Native policy/filter state живёт до retirement всех factories/FDs. */
typedef struct State
{
    int allowed; /* Live revocation gate, rechecked on already opened streams. */
    unsigned callbacks; /* Actual request preparation calls, no guest execution. */
    unsigned notifications; /* Native deferred-request notifications. */
    unsigned writes; /* Accepted scanner writes; refused payloads must not increment. */
    unsigned destroyed; /* Final scanner factory retirement count. */
    DVM_Provider *prepared; /* Borrowed selected tree while publisher owns calls. */
} State;


/* EN/RU: Fail explicitly; no build-mode assertions.
 * Явный failure, без build-mode assertions. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "export: %s\n", label);
        exit(2);
    }
}


/* EN/RU: Only approved peers enter the publication path, independently of PREPARE.
 * Только approved peers допускаются, независимо от PREPARE. */
static int authorize(void *context, const DVM_ExportRequest *request)
{
    (void)context;
    return !strcmp(request->peer, "alice");
}


/* EN/RU: Live tree policy; it can revoke already opened resource operations.
 * Live tree policy может отозвать права у уже открытых ресурсов. */
static int policy(void *context, const char *path, unsigned access)
{
    State *state = context;
    (void)path;
    (void)access;
    return state->allowed;
}


/* EN/RU: Fixed opaque endpoint metadata, usable as file/channel/function facade.
 * Metadata fixed opaque endpoint, пригодного как file/channel/function facade. */
static FVM_Result info(void *context, const char *path, FVM_FileInfo *output)
{
    (void)context;
    if (strcmp(path, "scan"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->mode = 0666;
    return FVM_OK;
}


/* EN/RU: Scanner facade emits one identifying byte for ordinary reads.
 * Scanner facade выдаёт identifying byte для обычного read. */
static FVM_Result read_file(void *context, void *bytes, size_t size, size_t *received)
{
    (void)context;
    *received = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = 'F';
    }
    return FVM_OK;
}


/* EN/RU: Test-only content predicate, not an antivirus engine; reject before effects.
 * Test-only predicate, не antivirus engine; refusal до effects. */
static void write_file(void *context, const void *bytes, size_t size, DVM_IoResult *result)
{
    State *state = context;
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    if (size == 5 && !memcmp(bytes, "virus", size))
    {
        result->status = FVM_ERROR;
        result->stop = DVM_IO_DENIED;
        return;
    }
    result->status = FVM_OK;
    result->count = size;
    state->writes++;
}


/* EN/RU: Inner facade owns no state; outer wrapper retains the factory.
 * Inner facade не владеет state; outer wrapper удерживает factory. */
static void close_file(void *context)
{
    (void)context;
}
static const DVM_VfsOps file_ops = {NULL, read_file, NULL, NULL, NULL, close_file, NULL, NULL, write_file}; /* Opaque checking endpoint. */


/* EN/RU: Open the scanner-like endpoint through ordinary provider I/O.
 * Открыть scanner-like endpoint обычным provider I/O. */
static FVM_Result open_file(void *context, const char *path, const char *mode, DVM_VfsHandle *output)
{
    if (strcmp(path, "scan") || (strcmp(mode, "r") && strcmp(mode, "rw") && strcmp(mode, "wo")))
    {
        return FVM_UNSUPPORTED;
    }
    output->ops = &file_ops;
    output->state = context;
    return FVM_OK;
}


/* EN/RU: Fixed source effective access; view policy still restricts it separately.
 * Fixed source effective access, отдельно ограниченный view policy. */
static FVM_Result access_check(void *context, const char *path, unsigned access)
{
    (void)context;
    return !strcmp(path, "scan") && !(access & FVM_ACCESS_EXEC) ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Count final scanner retirement.
 * Посчитать final scanner retirement. */
static void destroy(void *context)
{
    ((State *)context)->destroyed++;
}
static const DVM_ProviderOps source_ops = {info, open_file, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Immutable endpoint identity. */


/* EN/RU: Prepare deliberately different resource, or defer to explicit owner reply.
 * Выдать намеренно другой resource либо отложить до explicit owner reply. */
static int prepare(void *context, const DVM_ExportRequest *request, DVM_Provider **chosen)
{
    State *state = context;
    state->callbacks++;
    require(!strcmp(request->peer, "alice") && request->kind == 2, "wrong intercepted requester/kind");
    if (!strcmp(request->name, "defer"))
    {
        return 0;
    }
    if (strcmp(request->name, "requested-original"))
    {
        return -1;
    }
    require(dvm_provider_retain(state->prepared), "chosen tree reference");
    *chosen = state->prepared;
    return 1;
}


/* EN/RU: Immediate native notification without target VM execution.
 * Немедленное native notification без исполнения target VM. */
static void notify(void *context)
{
    ((State *)context)->notifications++;
}


/* EN/RU: Observe ordinary tree bytes via the same provider ops used by VFS.
 * Читать обычные tree bytes через те же provider ops, что использует VFS. */
static void value(DVM_Provider *provider, const char *path, char expected)
{
    DVM_VfsHandle file = {0};
    char byte = 0;
    require(dvm_provider_open(provider, path, "r", &file) == FVM_OK, "ordinary exported file open");
    DVM_IoResult result = dvm_vfs_read_chunk(&file, &byte, 1);
    require(result.status == FVM_OK && result.count == 1 && byte == expected, "ordinary exported file data");
    dvm_vfs_close(&file);
}


/* EN/RU: Check filtered enumeration, including virtual containers and replacements.
 * Проверить filtered enumeration, virtual containers и replacements. */
static void listing(DVM_Provider *tree)
{
    DVM_VfsHandle directory = {0};
    FVM_FileInfo *entries;
    size_t count;
    unsigned names = 0;
    require(dvm_provider_open(tree, ".", "r", &directory) == FVM_OK, "tree directory open");
    for (unsigned batch = 0; batch < 8; batch++)
    {
        require(dvm_vfs_list(&directory, &entries, &count) == FVM_OK, "filtered directory read");
        if (!count)
        {
            break;
        }
        for (size_t index = 0; index < count; index++)
        {
            require(strcmp(entries[index].name, "secret") && strcmp(entries[index].name, "link") &&
                    strcmp(entries[index].name, "escape"), "hidden/symlink node leaked in list");
            unsigned bit = !strcmp(entries[index].name, "a") ? 1 : !strcmp(entries[index].name, "b") ? 2 :
                           !strcmp(entries[index].name, "added") ? 4 : !strcmp(entries[index].name, "container") ? 8 : 0;
            require(bit && !(names & bit), "duplicate/unexpected directory entry");
            names |= bit;
        }
        fvm_port_directory_entries_free(entries);
    }
    require(names == 15, "missing explicit or delegated node");
    dvm_vfs_close(&directory);
}


/* EN/RU: Verify explicit remote/local policy handoff and one-use grant tickets.
 * Проверить remote/local policy handoff и one-use grant tickets. */
static void publication(State *state)
{
    DVM_ExportOptions options = {2, DVM_EXPORT_PREPARE, NULL, authorize, prepare, notify, state};
    DVM_Publication *publication = dvm_publication_create(&options);
    DVM_ExportRequest request = {0};
    DVM_Provider *grant = NULL;
    strcpy(request.peer, "alice");
    strcpy(request.name, "requested-original");
    request.kind = 2;
    request.access = FVM_ACCESS_READ | FVM_ACCESS_EXEC;
    request.flags = DVM_EXPORT_REMOTE;
    require(publication && !dvm_publication_request(publication, &request), "unauthenticated remote request accepted");
    request.flags |= DVM_EXPORT_AUTHENTICATED;
    DVM_ExportTicket *ticket = dvm_publication_request(publication, &request);
    require(ticket && state->callbacks == 1 && dvm_export_take(ticket, &grant) == 1 && grant,
            "prepared remote grant missing");
    DVM_Provider *first_grant = grant;
    require(dvm_export_take(ticket, &grant) == -1, "grant redeemed twice");
    dvm_provider_release(first_grant);
    dvm_export_release(ticket);
    ticket = dvm_publication_request(publication, &request);
    require(ticket && dvm_export_take(ticket, &grant) == 1, "second explicit prepared grant");
    value(grant, "b", 'F');
    DVM_VfsHandle refused = {0};
    require(dvm_provider_open(grant, "b", "wo", &refused) != FVM_OK, "requested rights ceiling bypassed");
    dvm_provider_release(grant);
    dvm_export_release(ticket);
    strcpy(request.name, "defer");
    ticket = dvm_publication_request(publication, &request);
    require(ticket && dvm_export_take(ticket, &grant) == 0 && state->notifications == 1, "deferred handoff missing");
    DVM_ExportRequest review;
    require(dvm_publication_next(publication, &review) && !strcmp(review.name, "defer") &&
            !dvm_publication_next(publication, &request), "next did not return exactly one request");
    require(dvm_publication_reply(publication, review.id, state->prepared) &&
            dvm_export_take(ticket, &grant) == 1, "explicit owner reply failed");
    dvm_provider_release(grant);
    dvm_export_release(ticket);
    options.flags = 0;
    options.direct = state->prepared;
    options.prepare = NULL;
    DVM_Publication *direct = dvm_publication_create(&options);
    require(direct != NULL, "direct publication");
    strcpy(request.peer, "bob");
    ticket = dvm_publication_request(direct, &request);
    require(ticket && dvm_export_take(ticket, &grant) == 2, "direct mode bypassed admission");
    dvm_export_release(ticket);
    strcpy(request.peer, "alice");
    ticket = dvm_publication_request(direct, &request);
    require(ticket && dvm_export_take(ticket, &grant) == 1, "prepublished direct tree missing");
    dvm_publication_close(direct);
    value(grant, "a", 'A');
    dvm_provider_release(grant);
    dvm_export_release(ticket);
    strcpy(request.name, "defer");
    ticket = dvm_publication_request(publication, &request);
    require(ticket && dvm_publication_next(publication, &review), "cancellation review");
    dvm_export_release(ticket);
    require(!dvm_publication_reply(publication, review.id, state->prepared), "cancelled request resurrected");
    ticket = dvm_publication_request(publication, &request);
    dvm_publication_close(publication);
    require(ticket && dvm_export_take(ticket, &grant) == 2, "close did not refuse unanswered request");
    dvm_export_release(ticket);
}


/* EN/RU: Ordinary anchors follow native symlinks; confined copies reject every
 * escaping form before read/write/access/removal. The original remains unchanged.
 * Ordinary anchors следуют symlinks; confined копии отклоняют escape до effects. */
static void confinement(DVM_ResourcePlan *plan)
{
    DVM_Anchor *ordinary = dvm_anchor_open(plan->anchor, plan->relative);
    DVM_Anchor *safe = ordinary ? dvm_anchor_confine(ordinary) : NULL;
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    FVM_PortAnchor *child = NULL;
    FVM_FileInfo metadata;
    require(safe && fvm_port_file_open_at(dvm_anchor_port(ordinary), "link", "r", &file) == FVM_OK,
            "ordinary native symlink control failed");
    fvm_port_file_close(file);
    file = NULL;
    require(fvm_port_file_open_at(dvm_anchor_port(safe), "link", "r", &file) != FVM_OK &&
            fvm_port_file_open_at(dvm_anchor_port(safe), "escape/private", "w", &file) != FVM_OK &&
            fvm_port_anchor_info_at(dvm_anchor_port(safe), "link", &metadata) != FVM_OK &&
            fvm_port_anchor_access_at(dvm_anchor_port(safe), "link", FVM_ACCESS_READ) != FVM_OK &&
            fvm_port_directory_open_at(dvm_anchor_port(safe), "escape", &directory) != FVM_OK &&
            fvm_port_anchor_open_at(dvm_anchor_port(safe), "escape", &child) != FVM_OK &&
            fvm_port_anchor_remove_at(dvm_anchor_port(safe), "escape/private") != FVM_OK &&
            fvm_port_anchor_remove_at(dvm_anchor_port(safe), "../outside") != FVM_OK,
            "confined capability followed an escape path");
    require(fvm_port_anchor_remove_at(dvm_anchor_port(safe), "remove-link") == FVM_OK,
            "confined unlink should remove only the symlink itself");
    dvm_anchor_release(safe);
    dvm_anchor_release(ordinary);
}


/* EN/RU: Real native fixture paths supplied by the temporary-tree driver.
 * Real native fixture paths задаются temporary-tree driver. */
int main(int argc, char **argv)
{
    test_export_hidden_batches();
    test_export_races();
    State state = {1, 0, 0, 0, 0, NULL};
    DVM_Namespace *group;
    DVM_ResourcePlan plan;
    require(argc == 2, "native fixture root argument");
    group = dvm_namespace_create(argv[1], "/", NULL);
    require(group && dvm_namespace_pin(group) && dvm_resource_plan(group, "/data", 0, &plan), "native root capture");
    confinement(&plan);
    DVM_Provider *base = dvm_export_native(&plan, 1);
    dvm_resource_close(&plan);
    DVM_Provider *scanner = dvm_provider_create(&source_ops, &state);
    DVM_View *view = dvm_view_create(policy, &state, NULL);
    require(base && scanner && view && dvm_view_map(view, "/", base, ".", 5, DVM_VIEW_TREE) &&
            dvm_view_map(view, "/secret", NULL, ".", 0, DVM_VIEW_TREE) &&
            dvm_view_map(view, "/b", scanner, "scan", 6, 0) &&
            dvm_view_map(view, "/added", scanner, "scan", 6, 0) &&
            dvm_view_map(view, "/container/input", scanner, "scan", 6, 0), "prepared mappings");
    state.prepared = dvm_view_freeze(view);
    require(state.prepared && dvm_provider_bindable(state.prepared), "frozen ordinary provider");
    value(state.prepared, "a", 'A');
    value(state.prepared, "b", 'F');
    value(state.prepared, "container/input", 'F');
    listing(state.prepared);
    DVM_VfsHandle denied = {0}, file = {0};
    require(dvm_provider_open(state.prepared, "secret", "r", &denied) != FVM_OK &&
            dvm_provider_open(state.prepared, "link", "r", &denied) != FVM_OK &&
            dvm_provider_open(state.prepared, "escape/private", "r", &denied) != FVM_OK &&
            dvm_provider_open(state.prepared, "../outside", "r", &denied) != FVM_OK, "hidden or escaping resource opened");
    require(dvm_provider_open(state.prepared, "b", "rw", &file) == FVM_OK, "checking endpoint open");
    DVM_IoResult result = dvm_vfs_write_chunk(&file, "virus", 5);
    require(result.status != FVM_OK && !result.count && !state.writes, "filter allowed refused content");
    result = dvm_vfs_write_chunk(&file, "safe", 4);
    require(result.status == FVM_OK && result.count == 4 && state.writes == 1, "checking endpoint did not forward safe input");
    state.allowed = 0;
    char refused_byte = 0;
    result = dvm_vfs_read_chunk(&file, &refused_byte, 1);
    require(result.status != FVM_OK && result.stop == DVM_IO_DENIED && !result.count, "old FD bypassed live policy");
    state.allowed = 1;
    dvm_vfs_close(&file);
    DVM_ChangeCursor empty = {0};
    DVM_ChangeBatch before, after;
    require(dvm_changes_pending(dvm_provider_changes(state.prepared), &empty, DVM_CHANGE_ALL, &before) == 1, "view journal");
    DVM_ChangeEvent event = {1, 2, DVM_CHANGE_RIGHTS, 0, 0};
    require(dvm_changes_notify(dvm_provider_changes(scanner), &event) == 1 &&
            dvm_changes_pending(dvm_provider_changes(state.prepared), &empty, DVM_CHANGE_ALL, &after) == 1 &&
            after.event.generation > before.event.generation, "underlying changes not forwarded automatically");
    publication(&state);
    require(dvm_stack_mount(group, state.prepared, "/mount", DVM_NS_INSERT, 0) == 1, "combined mount and indexed bind");
    require(dvm_resource_plan(group, "/mount/b", 0, &plan) && dvm_union_select(&plan, 0) == FVM_OK, "mounted ordinary lookup");
    value(plan.provider, plan.relative, 'F');
    dvm_resource_close(&plan);
    dvm_namespace_release(group);
    dvm_provider_release(state.prepared);
    dvm_provider_release(scanner);
    dvm_provider_release(base);
    require(state.destroyed == 1, "export/provider/stream/ticket lifetime leak");
    puts("PASS prepared ordinary tree, hidden/add/replace/function nodes, confined paths, live ACL, remote prepare/direct/deferred/cancel/close and mount+bind");
    return 0;
}
