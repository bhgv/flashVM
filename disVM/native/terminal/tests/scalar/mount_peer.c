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

static int calls; /* Inspected only after the provider worker joins. */


/* EN: Confirm the selected .self exposes the calling process's live PID over Styx.
 * RU: Проверить текущий PID клиента через выбранный .self на встречном Styx. */
static int compute(void *state, DVM_Provider *self, int64_t value, int64_t *out)
{
    (void)state;
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
    int ready = styxaddfile(&server, Qroot, 1, "app.dis", 0555, "provider") &&
        styxaddfile(&server, Qroot, 2, "other.dis", 0555, "provider") &&
        styxaddfile(&server, Qroot, 3, "submit", 0222, "provider");
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
    DVM_Provider *root = reverse ? styxmountprovider(reverse, FVM_ACCESS_READ) : NULL;
    StyxScalarFunction function = {.request_tag = STYX_FUNCTION_CALLER_TAG,
        .reply_tag = STYX_FUNCTION_REPLY_TAG, .program_root = root, .program_method = compute};
    StyxUnibusServer *server = root ?
        styxunibusserverstartfunction(phybidirchannel(mux, 0), "compute.dis", "compute", &function) : NULL;
    int success = server && styxunibusserverjoin(server);
    dvm_provider_release(root);
    phybidirfree(mux);
    return success && calls == 2 ? 0 : 1;
}
