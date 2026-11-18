/* Copyright (c) 2026 bhgv
 * EN: Native live-root fixture shared by SDK and actual guest launch-environment tests.
 * RU: Native live-root fixture для SDK и настоящих guest launch-environment tests.
 */
#include "../../../runtime/vfs/launch/prepare.h"
#include "../../../runtime/vfs/internal.h"
#include "../../../runtime/vfs/stack.h"
#include "../../../runtime/vfs/cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Caller owns state until every root and opened handle retires.
 * Caller владеет state до завершения root/handle lifetime. */
typedef struct LaunchState
{
    char value; /* Live camera/keyboard payload, independent from route identity. */
    int terminal; /* Whether server-only path is absent here. */
    int cancel_root; /* Request borrowed source during root validation. */
    int file_root; /* Deliberate wrong-kind preparation failure. */
    unsigned roots; /* Actual root metadata callbacks. */
    unsigned denied; /* Actual denied-path metadata callbacks. */
    unsigned destroyed; /* Final factory destructor count. */
    DVM_Cancel *cancel; /* Borrowed source through explicit native scenario. */
} LaunchState;


/* EN/RU: Fail fixture contract explicitly in every build mode.
 * Явно завершить fixture при нарушении contract. */
static void launch_check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "launch environment: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Return current directory/file metadata; denied terminal never means absent.
 * Вернуть live metadata; terminal denial не означает отсутствие. */
static FVM_Result launch_info(void *opaque, const char *path, FVM_FileInfo *out)
{
    LaunchState *state = opaque;
    int root = !strcmp(path, ".");
    if (root)
    {
        state->roots++;
        if (state->cancel_root)
        {
            dvm_cancel_request(state->cancel);
        }
    }
    if (!strcmp(path, "denied"))
    {
        state->denied++;
        if (state->terminal)
        {
            return FVM_ERROR;
        }
    }
    if ((state->terminal && !strcmp(path, "server")) ||
        (!root && strcmp(path, "dev") && strcmp(path, "dev/camera") && strcmp(path, "dev/keyboard") &&
         strcmp(path, "server") && strcmp(path, "denied")))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = (root && !state->file_root) || !strcmp(path, "dev");
    out->mode = out->directory ? 0555 : 0444;
    out->identity.low = state->terminal ? 1 : 2;
    strcpy(out->name, root ? "/" : path);
    return FVM_OK;
}


/* EN/RU: Read live payload through held provider identity; no source snapshot.
 * Читать live payload через held provider, без source snapshot. */
static FVM_Result launch_read(void *opaque, void *bytes, size_t size, size_t *count)
{
    LaunchState *state = opaque;
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = state->value;
    }
    return FVM_OK;
}


/* EN/RU: Inner handle borrows state through outer provider pin; no owned allocation.
 * Inner handle заимствует state через provider pin, allocation не владеет. */
static void launch_close(void *state)
{
    (void)state;
}
static const DVM_VfsOps launch_stream = {.read = launch_read, .close = launch_close}; /* Live byte input. */


/* EN/RU: Open only existing files; transfer borrowed state under provider wrapper pin.
 * Открыть только existing files; state удерживает provider wrapper. */
static FVM_Result launch_open(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo info;
    if (strcmp(mode, "r") || launch_info(state, path, &info) != FVM_OK || info.directory)
    {
        return FVM_ERROR;
    }
    out->ops = &launch_stream;
    out->state = state;
    return FVM_OK;
}


/* EN/RU: Final native factory release records retirement; caller still owns stack state.
 * Final factory release отмечает retirement, stack state принадлежит caller. */
static void launch_destroy(void *opaque)
{
    LaunchState *state = opaque;
    state->destroyed++;
}
static const DVM_ProviderOps launch_ops = {.info = launch_info, .open = launch_open, .destroy = launch_destroy, .flags = DVM_PROVIDER_STABLE_BIND}; /* Stable roots, live children. */


/* EN/RU: Fill copied canonical IDs using deterministic bytes for tests, never credentials.
 * Заполнить canonical IDs детерминированными bytes для тестов, не credentials. */
static void launch_origin(DVM_Origin *origin, unsigned role, char value)
{
    memset(origin, 0, sizeof(*origin));
    origin->role = role;
    memset(origin->system, value, 42);
    origin->system[42] = 'A';
    memset(origin->export_id, value+1, 42);
    origin->export_id[42] = 'A';
    memset(origin->session, 'S', 42);
    origin->session[42] = 'A';
    strcpy(origin->nickname, role == DVM_ORIGIN_TERMINAL ? "terminal" : "provider");
}


/* EN/RU: Create two owned test factories and copied options; caller releases both.
 * Создать два owned factories и copied options; caller освобождает оба. */
static DVM_LaunchOptions launch_options(LaunchState *terminal, LaunchState *provider)
{
    DVM_LaunchOptions options = {0};
    options.policy = DVM_LAUNCH_TERMINAL_FIRST;
    options.terminal = dvm_provider_create_native(&launch_ops, terminal, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    options.provider = dvm_provider_create_native(&launch_ops, provider, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    launch_check(options.terminal && options.provider, "factory allocation");
    launch_origin(&options.terminal_origin, DVM_ORIGIN_TERMINAL, 'T');
    launch_origin(&options.provider_origin, DVM_ORIGIN_PROVIDER, 'P');
    return options;
}
