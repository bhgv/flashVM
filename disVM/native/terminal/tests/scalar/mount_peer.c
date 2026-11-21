/* Copyright (c) 2026 bhgv
 * EN: Provider process for the delegated-socket DisVM mount acceptance test.
 * RU: Процесс провайдера для проверки маунта DisVM через выданный сокет.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "launch_receive.h"
#include "caller_styx.h"
#include "unibus_server.h"
#include "scalar_function.h"
#include "scalar_verify.h"
#include "mount.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int calls; /* Inspected only after the provider worker joins. */


/* EN: Wait for two live PID directories while the initial function is still running.
 * RU: Дождаться двух живых каталогов PID пока первый вызов ещё выполняется. */
static int concurrent(DVM_Provider *root)
{
    const struct timespec pause = {0, 10000000};
    for (unsigned attempt = 0; attempt < 100; attempt++)
    {
        DVM_VfsHandle directory = {0};
        if (dvm_provider_open(root, ".prog", "r", &directory) == FVM_OK)
        {
            FVM_FileInfo *entries = NULL;
            size_t count = 0;
            FVM_Result status = dvm_vfs_list(&directory, &entries, &count);
            fvm_port_directory_entries_free(entries);
            dvm_vfs_close(&directory);
            if (status == FVM_OK && count == 2)
            {
                return 1;
            }
        }
        nanosleep(&pause, NULL);
    }
    return 0;
}


/* EN: Confirm the selected .self exposes the calling process's live PID over Styx.
 * RU: Проверить текущий PID клиента через выбранный .self на встречном Styx. */
static int compute(void *state, DVM_Provider *self, int64_t value, int64_t *out)
{
    if (!calls && !concurrent(state))
    {
        return 0;
    }
    DVM_VfsHandle reader = {0};
    if (value <= 0 || dvm_provider_open(self, ".pid", "r", &reader) != FVM_OK)
    {
        return 0;
    }
    char expected[22], bytes[22];
    int length = snprintf(expected, sizeof(expected), "%" PRId64 "\n", value);
    DVM_IoResult result = dvm_vfs_read_chunk(&reader, bytes, sizeof(bytes));
    dvm_vfs_close(&reader);
    if (result.status != FVM_OK || result.count != (size_t)length || memcmp(bytes, expected, result.count))
    {
        return 0;
    }
    ++calls;
    *out = value;
    return 1;
}


static int wide_calls; /* Successful wide calls after checking the reverse context. */


static int raw_calls; /* Full-width calls on the export without caller publication. */


/* EN: Echo the exact signed value on the unscoped export.
 * RU: Вернуть точное знаковое число из экспорта без контекста. */
static int raw(void *state, int64_t value, int64_t *out)
{
    (void)state;
    ++raw_calls;
    *out = value;
    return 1;
}


/* EN: Echo a full-width signed argument from the granted process context.
 * RU: Вернуть полный знаковый аргумент из выданного контекста процесса. */
static int wide(void *state, DVM_Provider *self, int64_t value, int64_t *out)
{
    (void)state;
    FVM_FileInfo metadata;
    if (dvm_provider_info(self, ".pid", &metadata) != FVM_OK || metadata.directory)
    {
        return 0;
    }
    ++wide_calls;
    *out = value;
    return 1;
}


static int pair_calls; /* Tuple dispatches, including the expected failure. */


/* EN: Validate the live caller scope and change one tuple component.
 * RU: Проверить живой контекст и изменить компонент кортежа. */
static int pair(void *state, DVM_Provider *self, const StyxTuple *argument, StyxTuple *reply)
{
    (void)state;
    FVM_FileInfo metadata;
    if (dvm_provider_info(self, ".pid", &metadata) != FVM_OK || metadata.directory)
    {
        return 0;
    }
    ++pair_calls;
    if (!argument->narrow || argument->narrow == INT32_MAX)
    {
        return 0;
    }
    *reply = (StyxTuple){.narrow = argument->narrow + 1, .wide = argument->wide};
    return 1;
}


static int ref_calls; /* Verified ref dispatch count. */


/* EN: Return the old value and propose an update, even for the expected error.
 * RU: Вернуть прежнее значение и предложить обновление, включая ожидаемую ошибку. */
static int revise(void *state, DVM_Provider *self, const StyxTuple *argument, StyxRefReply *reply)
{
    (void)state;
    FVM_FileInfo metadata;
    if (dvm_provider_info(self, ".pid", &metadata) != FVM_OK || metadata.directory)
    {
        return 0;
    }
    ++ref_calls;
    reply->result = *argument;
    reply->updated = (StyxTuple){.narrow = 41, .wide = INT64_MAX};
    if (argument->narrow != 40)
    {
        reply->updated.narrow = -1;
        return 0;
    }
    return 1;
}


/* EN: Confirm the launch bytes accepted by the provider's ordinary request file.
 * RU: Проверить стартовые байты, принятые обычным файлом запросов провайдера. */
static int launch_payload(void *unused, const void *bytes, size_t size)
{
    (void)unused;
    return size == 3 && !memcmp(bytes, "a\0b", 3);
}

static StyxLaunchReceiver launch_receiver = { /* One scoped fixture endpoint. */
    .tag = 17, .start_payload = launch_payload, .payload_limit = 3
};
static int launched; /* Number of accepted launch records. */


/* EN: Accept one encoded launch record on the configured write-only file.
 * RU: Принять запись запуска на настроенном файле только для записи. */
static char *submit(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 3 || offset < 0 || !styxterminallaunchreceive(&launch_receiver, offset, bytes, *count))
    {
        return Ebadarg;
    }
    ++launched;
    return nil;
}


/* EN: Serve the launch fixture without exposing any code download dependency.
 * RU: Обслужить тест запуска без зависимости от скачивания кода. */
static int serve_launch(PhyBidir *mux)
{
    Styxserver server = {0};
    Styxops operations = {.write = submit};
    if (styxphyinit(&server, &operations, phybidirchannel(mux, 0), 0555, 1, 8))
    {
        return 0;
    }
    int ready = styxaddfile(&server, Qroot, 1, "implementation.dis", 0555, "provider") &&
        styxaddfile(&server, Qroot, 2, "other.dis", 0555, "provider") &&
        styxaddfile(&server, Qroot, 3, ".unibus", 0222, "provider");
    while (ready && !styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return ready && launched == 1;
}


/* EN: Consume one inherited carrier and serve until the caller closes it.
 * RU: Принять унаследованный транспорт и обслуживать до закрытия клиентом. */
int main(int argc, char **argv)
{
    if (argc != 2 && (argc != 3 || strcmp(argv[2], "launch")))
    {
        return 2;
    }
    unsigned flags = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    PhyBidir *mux = phybidircreate(phyposixfd(atoi(argv[1]), flags), 65536);
    if (argc == 3)
    {
        int success = mux && serve_launch(mux);
        phybidirfree(mux);
        return !success;
    }
    StyxClient *reverse = mux ? styxclientmountphy(phybidirchannel(mux, 1), 4096,
        "provider", "", 5000, 8) : NULL;
    DVM_Provider *root = reverse ? styxmountprovider(reverse, FVM_ACCESS_READ | FVM_ACCESS_EXEC) : NULL;
    StyxScalarFunction function = {.request_tag = STYX_FUNCTION_CALLER_TAG,
        .reply_tag = STYX_FUNCTION_REPLY_TAG, .state = root, .program_root = root, .program_method = compute};
    StyxScalarFunction big_function = {.request_tag = STYX_FUNCTION_BIG_CALLER_TAG,
        .reply_tag = STYX_FUNCTION_REPLY_TAG, .program_root = root, .program_method = wide};
    StyxScalarFunction raw_function = {.request_tag = STYX_FUNCTION_BIG_TAG,
        .reply_tag = STYX_FUNCTION_REPLY_TAG, .method = raw};
    StyxScalarFunction pair_function = {.request_tag = STYX_TUPLE_CALLER_TAG,
        .reply_tag = STYX_TUPLE_REPLY_TAG, .program_root = root, .tuple_method = pair};
    StyxScalarFunction ref_function = {.request_tag = STYX_REF_CALLER_TAG,
        .reply_tag = STYX_REF_REPLY_TAG, .program_root = root, .ref_method = revise};
    const StyxMountedScalar entries[] = {
        {"compute.dis", "compute", &function, 0, 0, 0, 0}, {"compute.dis", "wide", &big_function, 0, 0, 0, 0},
        {"compute.dis", "raw", &raw_function, 0, 0, 0, 0}, {"compute.dis", "pair", &pair_function, 0, 0, 0, 0},
        {"compute.dis", "revise", &ref_function, 0, 0, 0, 0}
    };
    StyxUnibusServer *server = root ?
        styxunibusserverstartmounted(phybidirchannel(mux, 0), entries, 5) : NULL;
    int success = server && styxunibusserverjoin(server);
    dvm_provider_release(root);
    phybidirfree(mux);
    return success && calls == 2 && wide_calls == 2 && raw_calls == 1 && pair_calls == 3 && ref_calls == 2 ? 0 : 1;
}
