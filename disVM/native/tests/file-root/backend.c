/* Copyright (c) 2026 bhgv
 * EN: One writable file exported directly or via native per-request preparation.
 * RU: Один writable file, публикуемый напрямую либо через native per-request preparation.
 */
#include "backend.h"
#include "peer_provider.h"
#include "../peer/review.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static DVM_Publication *publisher; /* Owned until reviewer joins and all peers retire. */
static DVM_Provider *provider; /* Owned single-file factory, independently pinned by grants/opens. */
static char value = 'Q'; /* Socket-owner serialized fixture file byte. */
static unsigned opens; /* Successful real ordinary file opens. */
static unsigned closes; /* Exactly-once ordinary handle retirement. */
static unsigned destroyed; /* Exactly-once original factory retirement. */


/* EN/RU: Always-active invariant with an explicit native diagnostic.
 * Постоянный invariant с явной native диагностикой. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "single-file backend: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Serve exactly one file root; no directory or descendant is exported.
 * Выдать ровно один file root; directory/descendants не экспортируются. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "remote");
    out->mode = 0666;
    out->identity.low = 42;
    out->device.low = 0x46494c45;
    out->length.low = 1;
    return FVM_OK;
}


/* EN/RU: Each actual open owns an independent sequential cursor.
 * Каждое actual open владеет independent sequential cursor. */
typedef struct Open
{
    size_t position; /* Exclusive byte offset for this open. */
} Open;


/* EN/RU: Return one byte or true EOF from this independent retained open.
 * Вернуть один byte либо true EOF из independent retained open. */
static void readchunk(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    Open *opened = state;
    memset(out, 0, sizeof(*out));
    out->status = FVM_OK;
    out->flags = DVM_IO_PREFIX;
    if (size && !opened->position)
    {
        *(char *)bytes = value;
        out->count = 1;
        opened->position++;
    }
}


/* EN/RU: A confirmed one-byte write mutates only this fixture file, never a namespace sibling.
 * Confirmed one-byte write меняет только fixture file, не namespace sibling. */
static void writechunk(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    Open *opened = state;
    memset(out, 0, sizeof(*out));
    out->status = FVM_OK;
    out->flags = DVM_IO_PREFIX;
    if (size && !opened->position)
    {
        value = *(const char *)bytes;
        out->count = 1;
        opened->position++;
    }
}


/* EN/RU: Consume actual cursor once; outer provider wrapper pins factory code/state.
 * Поглотить actual cursor один раз; outer wrapper удерживает factory code/state. */
static void closefile(void *state)
{
    closes++;
    free(state);
}
static const DVM_VfsOps opened = {NULL, NULL, NULL, NULL, NULL, closefile, NULL, readchunk, writechunk}; /* Precise native file chunks. */


/* EN/RU: Open the root only; native provider wrapper validates mode and pins lifetime.
 * Открыть только root; native provider wrapper проверяет mode и удерживает lifetime. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    (void)mode;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    out->state = calloc(1, sizeof(Open));
    if (!out->state)
    {
        return FVM_OUT_OF_MEMORY;
    }
    out->ops = &opened;
    opens++;
    return FVM_OK;
}


/* EN/RU: Final source retirement is visible after every remote fid/peer is gone.
 * Final source retirement виден после retirement всех remote fids/peers. */
static void destroy(void *state)
{
    (void)state;
    destroyed++;
}
static const DVM_ProviderOps operations = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND, NULL}; /* Stable single-file root. */


/* EN/RU: Fixture locally grants only its logical file service; not TCP authentication.
 * Fixture локально выдаёт только logical file service; это не TCP authentication. */
static int authorize(void *state, const DVM_ExportRequest *request)
{
    (void)state;
    return !strcmp(request->name, "file");
}


/* EN/RU: See backend.h; reviewer uses the same ordinary source as the direct publication.
 * См. backend.h; reviewer использует тот же ordinary source, что direct publication. */
void filerootsetup(int prepared)
{
    provider = dvm_provider_create(&operations, NULL);
    DVM_ExportOptions options = {0};
    options.limit = 8;
    options.flags = prepared ? DVM_EXPORT_PREPARE : 0;
    options.direct = provider;
    options.authorize = authorize;
    options.notify = prepared ? reviewnotify : NULL;
    publisher = dvm_publication_create(&options);
    check(provider && publisher, "publication");
    if (prepared)
    {
        check(!reviewstart(publisher, provider), "native reviewer");
        reviewpermit();
    }
}


/* EN/RU: See backend.h; explicit native local capability selects the authorization domain.
 * См. backend.h; явная native local capability выбирает authorization domain. */
StyxPeer *filerootpeer(void)
{
    return styxpeer(publisher, "fixture", 0, FVM_ACCESS_MASK);
}


/* EN/RU: See backend.h; verify original factory and every successful cursor retire exactly once.
 * См. backend.h; проверить exactly-once retirement factory и всех successful cursors. */
void filerootend(void)
{
    reviewstop();
    dvm_publication_close(publisher);
    reviewend();
    dvm_provider_release(provider);
    check(opens && opens == closes && destroyed == 1, "factory/cursor lifetime");
    puts("PASS direct/prepared single-file Styx publication");
}
