/* Copyright (c) 2026 bhgv
 * EN: Exchange a function call with the shared Dis owner service over local inboxes.
 * RU: Обмен вызовом с общим сервисом владельца Dis через локальные приёмники.
 */
#include "scalar_function_owner.h"
#include "scalar_route.h"
#include "scalar_verify.h"
#include <time.h>


/* EN/RU: Submit once and match the sole outstanding reply on the Styx dispatcher.
 * Отправить один раз и принять единственный ожидаемый ответ на диспетчере Styx. */
int styxscalarfunctiondispatch(void *state, int64_t argument, int64_t *value)
{
    StyxScalarFunctionOwner *owner = state;
    DVM_VfsHandle reader = {0};
    struct timespec start;
    if (owner->retired || clock_gettime(CLOCK_MONOTONIC, &start) ||
        dvm_provider_open(owner->reply, ".", "r", &reader) != FVM_OK)
    {
        return -1;
    }
    int result = -1;
    if (styxscalarcallfile(owner->request, STYX_FUNCTION_REQUEST_TAG, argument) == DVM_LAUNCH_DELIVERED)
    {
        struct timespec now;
        while (!clock_gettime(CLOCK_MONOTONIC, &now) &&
            (now.tv_sec - start.tv_sec) * INT64_C(1000) + (now.tv_nsec - start.tv_nsec) / 1000000 < 3000)
        {
            unsigned char record[STYX_SCALAR_RECORD_CAPACITY];
            DVM_IoResult got = dvm_vfs_read_chunk(&reader, record, sizeof(record));
            if (got.status == FVM_OK && (got.flags & DVM_IO_RECORD) && !(got.flags & DVM_IO_UNCERTAIN))
            {
                StyxScalarKind kind;
                if (styxscalarread(STYX_FUNCTION_REPLY_TAG, record, got.count, &kind, value))
                {
                    result = kind == STYX_SCALAR_VALUE ? 1 : kind == STYX_SCALAR_ERROR ? 0 : -1;
                }
                break;
            }
            if (got.stop != DVM_IO_AGAIN || got.count || (got.flags & DVM_IO_UNCERTAIN))
            {
                break;
            }
            const struct timespec delay = {0, 10000000};
            nanosleep(&delay, NULL);
        }
    }
    dvm_vfs_close(&reader);
    if (result < 0)
    {
        owner->retired = 1;
        styxunibusmailboxseal(owner->request);
    }
    return result;
}
