/* Copyright (c) 2026 bhgv
 * EN: Prepared terminal/provider union backed by confined real native files for wire acceptance.
 * RU: Prepared terminal/provider union над confined native files для wire acceptance.
 */
#include "service.h"
#include "protocol.h"
#include "offer.h"
#include "file_provider.h"
#include "launch/prepare.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Root
{
    FVM_PortAnchor *anchor; /* Owned confined directory capability. */
    char role; /* Fixture diagnostic only, never authority. */
} Root;
extern DVM_MovieOffer *movie_test_offers; /* Native job registry held through fixture lifetime. */
extern DVM_Namespace *movie_test_group; /* Private prepared namespace, not a wire pointer. */
extern DVM_Provider *movie_test_controlprovider(void); /* Exact native control grant. */
extern int movieserve(StyxBacking *root); /* Original libstyx translation unit. */


/* EN/RU: Inspect only a retained confined directory; borrow output, actual errors propagate.
 * Проверить retained confined directory; borrowed output, фактические ошибки сохраняются. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    return fvm_port_anchor_info_at(((Root *)state)->anchor, path, out);
}


typedef struct Partial
{
    DVM_VfsHandle inner; /* Owned actual file; fixture rejects after acknowledging valid bytes. */
} Partial;


/* EN/RU: Query the retained actual file, borrowing the output through return.
 * Запросить retained actual file, output borrowed до return. */
static FVM_Result partialinfo(void *state, FVM_FileInfo *out)
{
    return dvm_vfs_info(&((Partial *)state)->inner, out);
}


/* EN/RU: Confirm a complete valid SWF prefix and then report denial in the same operation.
 * Подтвердить полный валидный SWF prefix и сообщить отказ в той же операции. */
static void partialread(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    *out = dvm_vfs_read_chunk(&((Partial *)state)->inner, bytes, size);
    if (out->count)
    {
        fprintf(stderr, "PARTIAL %zu\n", out->count);
        out->status = FVM_ERROR;
        out->stop = DVM_IO_DENIED;
    }
}


/* EN/RU: Consume the owned actual file and its fixture wrapper exactly once.
 * Поглотить actual file и fixture wrapper ровно один раз. */
static void partialclose(void *state)
{
    Partial *partial = state;
    dvm_vfs_close(&partial->inner);
    free(partial);
}


static const DVM_VfsOps partialops = {
    partialinfo, NULL, NULL, NULL, NULL, partialclose, NULL, partialread, NULL
}; /* Independent raw file with deliberately denied acknowledged prefix. */


/* EN/RU: Open exactly once without host fallback; deliberately deny an existing fixture node.
 * Открыть ровно один раз без host fallback; существующий denied node явно запрещён. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    Root *root = state;
    FVM_PortFile *file = NULL;
    if (strcmp(mode, "r") || !strcmp(path, "denied.swf"))
    {
        return FVM_ERROR;
    }
    FVM_Result status = fvm_port_file_open_at(root->anchor, path, mode, &file);
    fprintf(stderr, "OPEN %c %s %d\n", root->role, path, status);
    if (status == FVM_OK && !dvm_vfs_adopt(out, file, NULL))
    {
        fvm_port_file_close(file);
        return FVM_ERROR;
    }
    if (status == FVM_OK && !strcmp(path, "partial.swf"))
    {
        Partial *partial = calloc(1, sizeof(*partial));
        if (!partial)
        {
            dvm_vfs_close(out);
            return FVM_OUT_OF_MEMORY;
        }
        dvm_vfs_move(&partial->inner, out);
        out->ops = &partialops;
        out->state = partial;
    }
    return status;
}


/* EN/RU: Consume root state only after namespace and all opened file references retire.
 * Поглотить root state после namespace и всех открытых файлов. */
static void destroy(void *state)
{
    Root *root = state;
    fvm_port_anchor_close(root->anchor);
    free(root);
}


/* EN/RU: Build an owned confined root;1/NULL failure leaves no directory descriptor live.
 * Создать owned confined root; NULL при отказе, без утечки directory descriptor. */
static DVM_Provider *rootprovider(const char *path, char role)
{
    static const DVM_ProviderOps ops = {info, openfile, destroy, 0, NULL};
    FVM_PortAnchor *anchor = NULL;
    Root *root = calloc(1, sizeof(*root));
    if (!root || fvm_port_anchor_open(path, &anchor) != FVM_OK)
    {
        free(root);
        return NULL;
    }
    FVM_Result status = fvm_port_anchor_confine(anchor, &root->anchor);
    fvm_port_anchor_close(anchor);
    if (status != FVM_OK)
    {
        free(root);
        return NULL;
    }
    root->role = role;
    DVM_Provider *provider = dvm_provider_create_native(&ops, root, NULL, 1, 0);
    if (!provider)
    {
        destroy(root);
    }
    return provider;
}


/* EN/RU: Copy deterministic test provenance, not credentials or public-key identities.
 * Копировать deterministic provenance теста, не credentials/public-key identities. */
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


/* EN/RU: Identify a complete END solely for deliberate wire truncation in the fixture.
 * Определить полный END только для намеренной wire truncation теста. */
int movie_test_end(const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t consumed;
    return fvm_srecord_read(bytes, size, MOVIE_FRAME, &record, &consumed) == FVM_S_OK &&
        consumed == size && record.tag == MOVIE_TAG && record.size && record.payload[0] == MOVIE_END;
}


/* EN/RU: Serve one actual native client over a prepared live union; consume all owned roots.
 * Обслужить native client над prepared live union и поглотить все owned roots. */
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        return 2;
    }
    DVM_LaunchOptions options = {0};
    options.policy = DVM_LAUNCH_TERMINAL_FIRST;
    options.terminal = rootprovider(argv[1], 'T');
    options.provider = rootprovider(argv[2], 'P');
    origin(&options.terminal_origin, DVM_ORIGIN_TERMINAL, 'T');
    origin(&options.provider_origin, DVM_ORIGIN_PROVIDER, 'P');
    DVM_Namespace *group = dvm_launch_prepare(&options);
    DVM_Provider *service = group ? styxmovieservice(group) : NULL;
    StyxBacking *backing = service ? styxbacking(service) : NULL;
    if (getenv("MOVIE_TEST_ANNOUNCE"))
    {
        movie_test_offers = dvm_movie_offer_create();
        movie_test_group = group;
        if (!movie_test_offers || dvm_movie_offer_bind(movie_test_offers, 0, service) != 1)
        {
            return 4;
        }
        DVM_Provider *control = movie_test_controlprovider();
        DVM_Provider *folder = control ? styxmovieofferroot(movie_test_offers, control) : NULL;
        styxbackingfree(backing);
        backing = folder ? styxtree(folder) : NULL;
        dvm_provider_release(folder);
        dvm_provider_release(control);
    }
    int result = backing ? movieserve(backing) : 3;
    dvm_movie_offer_release(movie_test_offers);
    styxbackingfree(backing);
    dvm_provider_release(service);
    dvm_namespace_release(group);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return result;
}
