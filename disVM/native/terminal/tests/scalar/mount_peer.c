/* Copyright (c) 2026 bhgv
 * EN: Provider process for the delegated-socket DisVM mount acceptance test.
 * RU: Процесс провайдера для проверки маунта DisVM через выданный сокет.
 */
#include <lib9.h>
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


/* EN: Consume one inherited carrier and serve until the caller closes it.
 * RU: Принять унаследованный транспорт и обслуживать до закрытия клиентом. */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 2;
    }
    unsigned flags = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    PhyBidir *mux = phybidircreate(phyposixfd(atoi(argv[1]), flags), 65536);
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
