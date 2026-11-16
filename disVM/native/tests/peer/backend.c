/* Copyright (c) 2026 bhgv
 * EN: Native admission/endpoint lifecycle oracle and distinct live connection roots.
 * RU: Проверка native admission/endpoint lifetime и отдельных корней соединений.
 */
#include "peer_provider.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Open Open;
struct Open
{
    const char *bytes; /* Borrowed immutable factory data, pinned by provider handle. */
    size_t position; /* Exclusive opened cursor. */
};
static DVM_Publication *publisher; /* Live fixture owner, consumed explicitly. */
static DVM_Provider *sources[2]; /* Owned native roots, with deliberately equal IDs. */
static int opens, closes, destroys, gates, notices; /* Serialized fixture counters. */
extern int peerwire(void);
static int authorize(void *, const DVM_ExportRequest *);
static void notify(void *);


/* EN/RU: Keep acceptance assertions enabled in optimized builds.
 * Сохранить проверки в оптимизированной сборке. */
static void check(int ok, const char *why)
{
    if (!ok)
    {
        fprintf(stderr, "peer bridge: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Resolve the fixture's single live file, without exposing host paths.
 * Разрешить единственный файл теста без host paths. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "file"))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".");
    out->identity.low = out->directory ? 1 : 2;
    out->device.low = 71;
    out->mode = out->directory ? 0555 : 0444;
    out->length.low = out->directory ? 0 : 5;
    strcpy(out->name, out->directory ? "." : "file");
    return FVM_OK;
}


/* EN/RU: Effective access is independent of displayed mode.
 * Effective access независимо от отображаемого mode. */
static FVM_Result accessnode(void *state, const char *path, unsigned access)
{
    FVM_FileInfo out;
    return !(access & FVM_ACCESS_WRITE) ? info(state, path, &out) : FVM_ERROR;
}


/* EN/RU: Return metadata of the retained actual file.
 * Вернуть metadata удержанного файла. */
static FVM_Result openinfo(void *state, FVM_FileInfo *out)
{
    return info(state, "file", out);
}


/* EN/RU: Read a confirmed sequential prefix; no metadata preflight is required.
 * Читать подтверждённый последовательный prefix без metadata preflight. */
static void readchunk(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    Open *opened = state;
    size_t n = opened->position >= 5 ? 0 : 5-opened->position;
    if (n > size)
    {
        n = size;
    }
    memset(out, 0, sizeof(*out));
    if (n)
    {
        memcpy(bytes, opened->bytes+opened->position, n);
    }
    opened->position += n;
    out->count = n;
    out->status = FVM_OK;
    out->flags = DVM_IO_PREFIX;
}


/* EN/RU: Consume the one native cursor before provider retirement.
 * Поглотить native cursor до освобождения provider. */
static void closefile(void *state)
{
    closes++;
    free(state);
}
static const DVM_VfsOps operations = {openinfo, NULL, NULL, NULL, NULL, closefile, NULL, readchunk, NULL};


/* EN/RU: Only a granted relative file supports opening; every fid owns a cursor.
 * Открывать только разрешённый относительный файл; каждый fid владеет cursor. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(path, "file") || strcmp(mode, "r"))
    {
        return FVM_ERROR;
    }
    Open *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    opened->bytes = state;
    out->state = opened;
    out->ops = &operations;
    opens++;
    return FVM_OK;
}


/* EN/RU: Count final factory retirement; fixture bytes are static storage.
 * Учесть финальное освобождение factory; bytes находятся в static storage. */
static void destroy(void *state)
{
    (void)state;
    destroys++;
}
static const DVM_ProviderOps factory = {info, openfile, destroy, 0, accessnode};


/* EN/RU: Admit only logical service requests with locally installed provenance.
 * Разрешить только service с установленным native-local provenance. */
static int authorize(void *state, const DVM_ExportRequest *request)
{
    (void)state;
    gates++;
    return !request->flags && !strcmp(request->name, "service") &&
        (!strcmp(request->peer, "slot-a") || !strcmp(request->peer, "slot-b"));
}


/* EN/RU: Choose root by native peer context; packet uname never reaches this gate.
 * Выбрать корень по native peer context; uname пакета не поступает в gate. */
static int prepare(void *state, const DVM_ExportRequest *request, DVM_Provider **chosen)
{
    (void)state;
    *chosen = sources[!strcmp(request->peer, "slot-b")];
    return dvm_provider_retain(*chosen) ? 1 : -1;
}


/* EN/RU: Native notification oracle, no guest polling/execution or suspension.
 * Native notification, без guest polling/execution или приостановок. */
static void notify(void *state)
{
    (void)state;
    notices++;
}


/* EN/RU: Test pending identity, cancellation, kind checking and endpoint after close.
 * Проверить pending identity, отмену, kind и endpoint после close. */
static void admissiontests(void)
{
    DVM_ExportOptions options = {0};
    DVM_ExportRequest request = {0};
    DVM_Publication *owner;
    StyxPeer *peer;
    StyxBacking *root = NULL, *child;
    StyxOpen *opened;
    StyxTransfer result;
    char bytes[5];
    options.limit = 4;
    options.flags = DVM_EXPORT_PREPARE;
    options.authorize = authorize;
    options.notify = notify;
    owner = dvm_publication_create(&options);
    check(owner != NULL, "deferred owner");
    ExportEndpoint *endpoint = exportendpoint(owner);
    check(endpoint != NULL, "submit-only endpoint");
    check(!styxpeer(owner, "slot-a", DVM_EXPORT_REMOTE, FVM_ACCESS_READ), "unattested remote rejected");
    check(!styxpeer(owner, "slot-a", 4, FVM_ACCESS_READ), "unknown provenance rejected");
    peer = styxpeer(owner, "slot-a", 0, FVM_ACCESS_READ|FVM_ACCESS_EXEC);
    check(peer != NULL, "deferred peer");
    check(styxpeerattach(peer, "service", 2, &root) == 0 && !root && notices == 1, "pending notification");
    check(styxpeerattach(peer, "other", 2, &root) == -1 && !root, "pending root substitution");
    check(styxpeerattach(peer, "service", 2, &root) == 0 && notices == 1, "no duplicate ticket");
    check(dvm_publication_next(owner, &request) == 1 && !strcmp(request.peer, "slot-a"), "trusted copied identity");
    check(dvm_publication_reply(owner, request.id, sources[0]), "owner chosen tree");
    check(styxpeerattach(peer, "service", 2, &root) == 1 && root, "redeem chosen root");
    child = styxwalknode(root, "file");
    check(child != NULL, "granted descendant");
    opened = styxfileopen(child, 0);
    check(opened != NULL, "held open");
    styxbackingfree(child);
    styxbackingfree(root);
    styxpeerfree(peer);
    peer = styxpeer(owner, "slot-a", 0, FVM_ACCESS_READ|FVM_ACCESS_EXEC);
    check(styxpeerattach(peer, "service", 2, &root) == 0 && dvm_publication_next(owner, &request), "cancel pending");
    styxpeerfree(peer);
    check(!dvm_publication_reply(owner, request.id, sources[0]), "late reply refused");
    peer = styxpeer(owner, "slot-a", 0, FVM_ACCESS_READ|FVM_ACCESS_EXEC);
    check(styxpeerattach(peer, "service", 2, &root) == 0, "pending close");
    dvm_publication_close(owner);
    check(styxpeerattach(peer, "service", 2, &root) == -1 && !root, "close denies pending");
    styxpeerfree(peer);
    int before = gates;
    memset(&request, 0, sizeof(request));
    strcpy(request.peer, "slot-a");
    strcpy(request.name, "service");
    request.access = FVM_ACCESS_READ;
    check(!exportrequest(endpoint, &request) && gates == before, "closed endpoint does not call borrowed callbacks");
    exportendpointfree(endpoint);
    styxfileread(opened, 0, bytes, sizeof(bytes), &result);
    check(result.success && result.count == 5 && !memcmp(bytes, "alpha", 5), "open survives peer and owner close");
    styxfileclose(opened);
}


/* EN/RU: A file grant is admitted through the same peer API; kind never widens it.
 * File grant проходит тот же peer API; kind не расширяет полномочия. */
static void filetests(void)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    check(view && dvm_view_map(view, "/", sources[1], "file", FVM_ACCESS_READ, 0), "file view");
    DVM_Provider *file = dvm_view_freeze(view);
    check(file != NULL, "prepared file");
    DVM_ExportOptions options = {0};
    options.limit = 2;
    options.direct = file;
    options.authorize = authorize;
    DVM_Publication *owner = dvm_publication_create(&options);
    StyxPeer *peer = styxpeer(owner, "slot-b", 0, FVM_ACCESS_READ);
    StyxBacking *first = NULL, *second = NULL;
    check(owner && peer, "file peer");
    check(styxpeerattach(peer, "other", 1, &first) == -1 && !first, "denied logical resource");
    check(styxpeerattach(peer, "service", 3, &first) == -1 && !first, "invalid kind");
    check(styxpeerattach(peer, "service", 2, &first) == -1 && !first, "file is not a directory grant");
    check(styxpeerattach(peer, "service", 1, &first) == 1 && first, "file grant after failed kind");
    int before = gates;
    check(styxpeerattach(peer, "service", 1, &second) == 1 && first != second && gates == before,
        "same selected file independent wrappers without new approval");
    StyxBacking *refused = first;
    check(styxpeerattach(peer, "other", 1, &refused) == -1 && !refused, "bound root cannot change");
    check(styxpeerattach(peer, "service", 0, &refused) == -1 && !refused, "bound kind cannot change");
    StyxOpen *opened = styxfileopen(first, 0);
    check(opened != NULL, "prepared file open");
    dvm_publication_close(owner);
    dvm_provider_release(file);
    styxpeerfree(peer);
    styxbackingfree(first);
    styxbackingfree(second);
    char bytes[5];
    StyxTransfer result;
    styxfileread(opened, 0, bytes, sizeof(bytes), &result);
    check(result.success && result.count == 5 && !memcmp(bytes, "bravo", 5), "file handle survives all issuers");
    styxfileclose(opened);
}


/* EN/RU: Native fixture connection slots are local grants, not TCP authentication.
 * Native slots теста — local grants, не аутентификация TCP. */
StyxPeer *newpeer(unsigned slot)
{
    return publisher ? styxpeer(publisher, slot == 0 ? "slot-a" : "slot-b", 0,
        FVM_ACCESS_READ|FVM_ACCESS_EXEC) : NULL;
}


/* EN/RU: Consume owner once; already granted roots/opens remain usable.
 * Поглотить owner один раз; выданные roots/opens остаются доступны. */
void closepublisher(void)
{
    dvm_publication_close(publisher);
    publisher = NULL;
}


/* EN/RU: Run native oracle then real TCP attach against the same library bridge.
 * Выполнить native-проверки и настоящий TCP attach через тот же library bridge. */
int main(void)
{
    sources[0] = dvm_provider_create(&factory, "alpha");
    sources[1] = dvm_provider_create(&factory, "bravo");
    check(sources[0] && sources[1], "two factories");
    admissiontests();
    filetests();
    DVM_ExportOptions options = {0};
    options.limit = 8;
    options.flags = DVM_EXPORT_PREPARE;
    options.authorize = authorize;
    options.prepare = prepare;
    publisher = dvm_publication_create(&options);
    check(publisher != NULL, "TCP publication");
    check(peerwire() == 0, "TCP server");
    closepublisher();
    dvm_provider_release(sources[0]);
    dvm_provider_release(sources[1]);
    check(opens == closes && opens >= 3 && destroys == 2, "all cursors/factories retired once");
    printf("PASS native peer admission: %d opens closed, two factories destroyed\n", closes);
    return 0;
}
