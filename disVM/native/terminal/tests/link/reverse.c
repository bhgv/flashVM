/* Copyright (c) 2026 bhgv
 * EN: Caller-side Styx export for the reverse launch completion test.
 * RU: Экспорт Styx вызывающего для проверки встречного результата запуска.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "../../../launch/request.h"
#include "unibus_inbox.h"
#include "file_provider.h"
#include <pthread.h>

static Styxserver reverse_server; /* Caller-owned export on logical channel 1. */
static pthread_t reverse_worker; /* Processes writes from the command owner. */
static int reverse_active; /* Thread must be joined before freeing the mux. */
static DVM_Provider *inbox; /* Owner reference retained through server shutdown. */
static StyxBacking *backing; /* Write-only export enforced by the Styx open handler. */
static int reverse_received; /* Updated only by server thread, read after join. */

/* EN/RU: Grant reverse clients only a write fid on the inbox.
 * Выдавать встречным клиентам только fid записи в приёмник. */
static char *reverse_open(Qid *qid, int mode)
{
    if (qid->path == Qroot)
    {
        return mode == OREAD ? nil : Eperm;
    }
    if (qid->path != 1 || mode != OWRITE)
    {
        return Eperm;
    }
    StyxOpen *opened = styxfileopen(backing, mode);
    if (!opened || styxsetaux(&reverse_server, opened) < 0)
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Publish the complete S-record in the same inbox that Limbo reads.
 * Опубликовать полную S-запись в том же приёмнике, который читает Limbo. */
static char *reverse_write(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer result;
    if (qid.path != 1 || offset < 0)
    {
        return Ebadarg;
    }
    styxfilewrite(styxgetaux(&reverse_server), offset, bytes, *count, &result);
    if (!result.success || !result.confirmed || result.uncertain || result.count != *count)
    {
        return Ebadarg;
    }
    reverse_received = 1;
    return nil;
}

static Styxops reverse_ops = {
    .open = reverse_open, .write = reverse_write, .freefid = styxfileclose
}; /* Caller-side one-shot export. */


/* EN/RU: Borrow the provider until the reverse server joins.
 * Заимствовать провайдер до завершения встречного сервера. */
DVM_Provider *fixturereverseinbox(void)
{
    return inbox;
}


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
    inbox = styxunibusinbox(17);
    backing = styxbacking(inbox);
    if (!backing || !styxaddfile(&reverse_server, Qroot, 1, STYX_UNIBUS_NODE, 0222, "fixture") ||
        pthread_create(&reverse_worker, nil, serve_reverse, nil))
    {
        styxend(&reverse_server);
        styxbackingfree(backing);
        dvm_provider_release(inbox);
        return 0;
    }
    reverse_active = 1;
    return 1;
}


/* EN/RU: Join after carrier shutdown and validate the received completion.
 * Дождаться сервера после закрытия транспорта и проверить полученный результат. */
int fixturereversefinish(void)
{
    if (!reverse_active)
    {
        return 1;
    }
    if (pthread_join(reverse_worker, nil))
    {
        return 0;
    }
    reverse_active = 0;
    styxbackingfree(backing);
    dvm_provider_release(inbox);
    backing = NULL;
    inbox = NULL;
    return reverse_received;
}
