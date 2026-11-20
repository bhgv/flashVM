/* Copyright (c) 2026 bhgv
 * EN: Caller-side Styx export for the reverse launch completion test.
 * RU: Экспорт Styx вызывающего для проверки встречного результата запуска.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "../../../launch/request.h"
#include "flashvm/srecord.h"
#include <pthread.h>

static Styxserver reverse_server; /* Caller-owned export on logical channel 1. */
static pthread_t reverse_worker; /* Processes writes from the command owner. */
static int reverse_active; /* Thread must be joined before freeing the mux. */
static int reverse_received; /* Updated only by server thread, read after join. */

/* EN/RU: Accept exactly one completion S-record on the caller's own .unibus file.
 * Принять одну S-запись завершения на собственном .unibus вызывающего. */
static char *reverse_write(Qid qid, char *bytes, ulong *count, vlong offset)
{
    FVM_SRecord record;
    size_t used;
    if (qid.path != 1 || offset || reverse_received ||
        fvm_srecord_read(bytes, *count, 32, &record, &used) != FVM_S_OK ||
        used != *count || record.tag != 17 || record.size != 2 || memcmp(record.payload, "0\n", 2))
    {
        return Ebadarg;
    }
    reverse_received = 1;
    return nil;
}

static Styxops reverse_ops = {.write = reverse_write}; /* Caller-side one-shot export. */


/* EN/RU: Serve the caller-side logical Styx channel until the carrier closes.
 * Обслужить логический канал Styx вызывающего до закрытия соединения. */
static void *serve_reverse(void *unused)
{
    USED(unused);
    while (!styxwait(&reverse_server) && !styxprocess(&reverse_server))
    {
    }
    styxend(&reverse_server);
    return nil;
}


/* EN/RU: Consume channel and start the caller's reverse server, returning 1 on success.
 * Принять канал и запустить встречный сервер клиента, вернуть 1 при успехе. */
int fixturereversestart(RemotePhy *channel)
{
    if (styxphyinit(&reverse_server, &reverse_ops, channel, 0555, 1, 2))
    {
        return 0;
    }
    if (!styxaddfile(&reverse_server, Qroot, 1, STYX_UNIBUS_NODE, 0222, "fixture") ||
        pthread_create(&reverse_worker, nil, serve_reverse, nil))
    {
        styxend(&reverse_server);
        return 0;
    }
    reverse_active = 1;
    return 1;
}


/* EN/RU: Join after carrier shutdown and validate the received completion.
 * Дождаться сервера после закрытия транспорта и проверить полученный результат. */
int fixturereversefinish(void)
{
    return !reverse_active || (!pthread_join(reverse_worker, nil) && reverse_received);
}
