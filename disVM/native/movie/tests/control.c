/* Copyright (c) 2026 bhgv
 * EN: Real Styx control-file fixture exposes automatic announcements and explicit test scope changes.
 * RU: Настоящий Styx control-file fixture выдаёт объявления и явные test scope changes.
 */
#include "writer.h"
#include "service.h"
#include "resource.h"
#include <stdlib.h>
#include <string.h>
DVM_MovieOffer *movie_test_offers; /* Owned by backend main through client retirement. */
DVM_Namespace *movie_test_group; /* Borrowed through backend main, never a guest pointer. */


typedef struct Control
{
    unsigned char prelude[128]; /* Fixture-only unrelated S-record pair. */
    size_t prelude_size; /* Complete encoded extent, set before exposing the control. */
    size_t prelude_sent; /* Confirmed ordinary Tread prefix. */
    int faulted; /* Fixture-only one-shot framing/EOF injection. */
    StyxMovieWriter *writer; /* Owned production serializer over autonomous source notices. */
} Control;
static Control *active; /* Last read callback context, borrowed only on the socket owner. */


typedef struct Output
{
    void *bytes; /* Borrowed Tread buffer until copy callback returns. */
    size_t size; /* Actual client-requested capacity. */
} Output;


/* EN/RU: Copy confirmed bytes into one real Tread response, preserving arbitrary fragmentation.
 * Копировать confirmed bytes в настоящий Tread response с произвольной fragmentation. */
static void copy(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    Output *output = state;
    size_t count = size < output->size ? size : output->size;
    memcpy(output->bytes, bytes, count);
    memset(out, 0, sizeof(*out));
    out->flags = DVM_IO_PREFIX;
    out->status = FVM_OK;
    out->count = count;
}


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
static void readcontrol(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    Control *control = state;
    active = control;
    if (!size)
    {
        memset(out, 0, sizeof(*out));
        out->flags = DVM_IO_PREFIX;
        out->status = FVM_OK;
        return;
    }
    const char *case_mode = getenv("MOVIE_TEST_ANNOUNCE");
    if (case_mode && !strncmp(case_mode, "monitor-", 8) && size > 3)
    {
        size = 3;
    }
    if (control->prelude_sent < control->prelude_size)
    {
        size_t count = control->prelude_size-control->prelude_sent;
        if (count > size)
        {
            count = size;
        }
        memcpy(bytes, control->prelude+control->prelude_sent, count);
        control->prelude_sent += count;
        memset(out, 0, sizeof(*out));
        out->flags = DVM_IO_PREFIX;
        out->status = FVM_OK;
        out->count = count;
        return;
    }
    if (case_mode && !strcmp(case_mode, "monitor-eof"))
    {
        memset(out, 0, sizeof(*out));
        out->flags = DVM_IO_PREFIX;
        out->status = FVM_OK;
        out->stop = DVM_IO_EOF;
        return;
    }
    Output output = {bytes, size};
    *out = styxmoviewriterstep(control->writer, copy, &output);
    if (case_mode && !strcmp(case_mode, "monitor-crc") && out->count && !control->faulted)
    {
        ((unsigned char *)bytes)[0] ^= 1;
        control->faulted = 1;
    }
}


/* EN/RU: Fixture-only authenticated host commands change scope or retire it; never SWF requests.
 * Только fixture host commands меняют scope/retire; это не SWF requests. */
FVM_Result movie_test_change(const void *bytes, size_t size)
{
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
    return FVM_OK;
}


/* EN/RU: Fixture command stream delegates only source mutation, never announcement encoding.
 * Fixture command stream меняет только source, не кодирует announcement. */
static FVM_Result writecontrol(void *state, const void *bytes, size_t size)
{
    (void)state;
    return movie_test_change(bytes, size);
}


/* EN/RU: Borrow source readiness while the pending fid pins its control writer; fixture-only.
 * Borrow readiness под pin pending fid; только fixture owner, не native producer. */
int movie_test_readyfd(void)
{
    return active ? styxmoviewriterfd(active->writer) : -1;
}


/* EN/RU: Consume one independent fixture stream after all fid borrowers retire.
 * Поглотить independent fixture stream после завершения fid borrowers. */
static void closecontrol(void *state)
{
    Control *control = state;
    if (active == control)
    {
        active = NULL;
    }
    styxmoviewriterfree(control->writer);
    free(control);
}
static const DVM_VfsOps operations = {
    info, NULL, writecontrol, NULL, NULL, closecontrol, NULL, readcontrol, NULL
}; /* Native fixture byte stream; no VM/SDL state. */


/* EN/RU: Factory metadata has the same immutable file identity as every open.
 * Factory metadata совпадают с immutable identity каждого open. */
static FVM_Result factoryinfo(void *state, const char *path, FVM_FileInfo *out)
{
    return !strcmp(path, ".") ? info(state, out) : FVM_NOT_FOUND;
}


/* EN/RU: Prepare an unrelated bound S-record for the generic native dispatcher test, no VM.
 * Подготовить unrelated bound S-record для generic native dispatcher test, без VM. */
static int prelude(Control *control)
{
    FVM_SBinding binding = {0};
    unsigned char payload[64], value = 42;
    size_t size, first, second;
    binding.tag = 9;
    binding.version = 1;
    binding.type_size = sizeof("flashvm.test")-1;
    memcpy(binding.type, "flashvm.test", sizeof("flashvm.test"));
    if (fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &size) != FVM_S_OK ||
        fvm_srecord_write(0, payload, size, control->prelude, sizeof(control->prelude), &first) != FVM_S_OK ||
        fvm_srecord_write(9, &value, 1, control->prelude+first, sizeof(control->prelude)-first, &second) != FVM_S_OK)
    {
        return 0;
    }
    control->prelude_size = first+second;
    return 1;
}


/* EN/RU: Open independent control receiver over a ready native source; output owned on success.
 * Открыть independent control receiver над ready native source; output owned при успехе. */
static FVM_Result opencontrol(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)state;
    if (strcmp(path, ".") || (strcmp(mode, "rw") && strcmp(mode, "r")))
    {
        return FVM_ERROR;
    }
    Control *control = calloc(1, sizeof(*control));
    if (!control)
    {
        return FVM_OUT_OF_MEMORY;
    }
    const char *case_mode = getenv("MOVIE_TEST_ANNOUNCE");
    if (case_mode && !strcmp(case_mode, "monitor-mixed") && !prelude(control))
    {
        free(control);
        return FVM_ERROR;
    }
    control->writer = styxmoviewriter(movie_test_offers, 7);
    if (!control->writer)
    {
        free(control);
        return FVM_OUT_OF_MEMORY;
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
