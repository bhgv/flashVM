/* Copyright (c) 2026 bhgv
 * EN: Real Styx control-file fixture exposes automatic announcements and explicit test scope changes.
 * RU: Настоящий Styx control-file fixture выдаёт объявления и явные test scope changes.
 */
#include "offer.h"
#include "service.h"
#include "resource.h"
#include <stdlib.h>
#include <string.h>
DVM_MovieOffer *movie_test_offers; /* Owned by backend main through client retirement. */
DVM_Namespace *movie_test_group; /* Borrowed through backend main, never a guest pointer. */


typedef struct Control
{
    unsigned char bytes[MOVIE_OFFER_FRAME*2]; /* One latest complete declaration/state pair. */
    size_t size; /* Complete pending extent. */
    size_t sent; /* Confirmed prefix. */
} Control;


/* EN/RU: Describe the test's granted control stream, never a host file.
 * Описать granted control stream теста, не host file. */
static FVM_Result info(void *state, FVM_FileInfo *out)
{
    (void)state;
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "control");
    out->mode = 0666;
    out->identity.low = 77;
    return FVM_OK;
}


/* EN/RU: Read arbitrary native prefixes, allowing the receiver to exercise record fragmentation.
 * Читать произвольные native prefixes для проверки record fragmentation. */
static FVM_Result readcontrol(void *state, void *bytes, size_t size, size_t *received)
{
    Control *control = state;
    size_t count = control->size-control->sent;
    if (count > size)
    {
        count = size;
    }
    memcpy(bytes, control->bytes+control->sent, count);
    control->sent += count;
    *received = count;
    return FVM_OK;
}


/* EN/RU: Fixture-only authenticated host commands change scope or retire it; never SWF requests.
 * Только fixture host commands меняют scope/retire; это не SWF requests. */
static FVM_Result writecontrol(void *state, const void *bytes, size_t size)
{
    Control *control = state;
    DVM_MovieOfferInfo current;
    dvm_movie_offer_info(movie_test_offers, &current);
    if (size == 7 && !memcmp(bytes, "replace", 7))
    {
        DVM_Namespace *copy = dvm_namespace_copy(movie_test_group);
        DVM_Provider *root = copy ? styxmovieservice(copy) : NULL;
        uint64_t generation = root ? dvm_movie_offer_bind(movie_test_offers, current.generation, root) : 0;
        dvm_provider_release(root);
        dvm_namespace_release(copy);
        if (!generation)
        {
            return FVM_ERROR;
        }
    }
    else if (size == 5 && !memcmp(bytes, "close", 5))
    {
        if (!dvm_movie_offer_close(movie_test_offers, current.generation))
        {
            return FVM_ERROR;
        }
    }
    else
    {
        return FVM_ERROR;
    }
    control->sent = 0;
    return styxmovieofferframe(movie_test_offers, 7, control->bytes, sizeof(control->bytes), &control->size);
}


/* EN/RU: Consume one independent fixture stream after all fid borrowers retire.
 * Поглотить independent fixture stream после завершения fid borrowers. */
static void closecontrol(void *state)
{
    free(state);
}
static const DVM_VfsOps operations = {
    info, readcontrol, writecontrol, NULL, NULL, closecontrol, NULL, NULL, NULL
}; /* Native fixture byte stream; no VM/SDL state. */


/* EN/RU: Factory metadata has the same immutable file identity as every open.
 * Factory metadata совпадают с immutable identity каждого open. */
static FVM_Result factoryinfo(void *state, const char *path, FVM_FileInfo *out)
{
    return !strcmp(path, ".") ? info(state, out) : FVM_NOT_FOUND;
}


/* EN/RU: Open independent control receiver over a ready native source; output owned on success.
 * Открыть independent control receiver над ready native source; output owned при успехе. */
static FVM_Result opencontrol(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    if (strcmp(path, ".") || strcmp(mode, "rw"))
    {
        return FVM_ERROR;
    }
    Control *control = calloc(1, sizeof(*control));
    if (!control)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Result result = styxmovieofferframe(movie_test_offers, 7,
        control->bytes, sizeof(control->bytes), &control->size);
    if (result != FVM_OK)
    {
        free(control);
        return result;
    }
    out->ops = &operations;
    out->state = control;
    return FVM_OK;
}


/* EN/RU: Factory owns no additional state beyond the fixture main's explicit lifetime.
 * Factory не владеет дополнительным state; lifetime явно удерживает fixture main. */
static void destroy(void *state)
{
    (void)state;
}


/* EN/RU: Owned exact control provider for the production directory projection.
 * Свой exact control provider для production directory projection. */
DVM_Provider *movie_test_controlprovider(void)
{
    static const DVM_ProviderOps factory = {factoryinfo, opencontrol, destroy, 0, NULL};
    return dvm_provider_create_native(&factory, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
}
