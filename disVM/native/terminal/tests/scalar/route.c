/* Copyright (c) 2026 bhgv
 * EN: Native scalar round trip over two original Styx sessions on one carrier.
 * RU: Скалярный обмен через две сессии исходного Styx на одном соединении.
 */
#include <sys/socket.h>
#include <unistd.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "scalar_route.h"
#include "unibus_server.h"
#include "mount.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static Styxserver owner; /* One fixture owner thread dispatches the method. */
static DVM_Provider *reply; /* Retained reverse file while owner serves requests. */
static int calls; /* Updated by owner and checked after join. */


/* EN/RU: Stop the isolated test process on a failed protocol invariant.
 * Остановить отдельный тестовый процесс при нарушении протокола. */
static void check(int valid, const char *label)
{
    if (!valid)
    {
        fprintf(stderr, "scalar route: %s\n", label);
        exit(1);
    }
}


/* EN/RU: Compute a fixture result or explicit error without unchecked overflow.
 * Вычислить тестовый результат или явную ошибку без переполнения. */
static int method(void *state, int64_t argument, int64_t *result)
{
    (void)state;
    calls++;
    *result = argument < 0 || argument == INT64_MAX ? 42 : argument + 1;
    return argument >= 0 && argument != INT64_MAX;
}


/* EN/RU: Validate the request and send a separate S-record through the reverse session.
 * Проверить запрос и отправить отдельную S-запись через встречную сессию. */
static char *receive_call(Qid qid, char *bytes, ulong *count, vlong offset)
{
    return qid.path == 1 && offset == 0 &&
        styxscalarreplyfile(reply, 17, bytes, *count, 29, method, NULL) == DVM_LAUNCH_DELIVERED ? nil : Ebadarg;
}

static Styxops operations = {.write = receive_call}; /* Native launch test callbacks. */


/* EN/RU: Run the original owner dispatcher until the carrier closes.
 * Обслуживать исходный сервер владельца до закрытия соединения. */
static void *serve(void *state)
{
    (void)state;
    while (!styxwait(&owner) && !styxprocess(&owner))
    {
    }
    styxend(&owner);
    return NULL;
}


/* EN/RU: Run one isolated call with its own request and reply capabilities.
 * Выполнить один изолированный вызов с собственными файлами запроса и ответа. */
static void round_trip(int64_t argument, StyxScalarKind expected_kind, int64_t expected)
{
    int pair[2];
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "connected socketpair");
    int breaker = dup(pair[0]);
    check(breaker >= 0, "shutdown descriptor");
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    PhyBidir *caller_mux = phybidircreate(phyposixfd(pair[0], caps), 1024);
    PhyBidir *owner_mux = phybidircreate(phyposixfd(pair[1], caps), 1024);
    check(caller_mux && owner_mux, "bidirectional carrier");
    StyxUnibusServer *receiver = styxunibusserverstart(phybidirchannel(caller_mux, 1), 29);
    check(receiver != NULL, "caller reverse .unibus");
    StyxClient *reverse = styxclientmountphy(phybidirchannel(owner_mux, 1), 256, "owner", "", 3000, 4);
    reply = reverse ? styxmountproviderat(reverse, ".unibus", FVM_ACCESS_WRITE) : NULL;
    check(reply != NULL, "mount caller reply file");
    memset(&owner, 0, sizeof(owner));
    check(!styxphyinit(&owner, &operations, phybidirchannel(owner_mux, 0), 0555, 1, 2) &&
        styxaddfile(&owner, Qroot, 1, ".unibus", 0222, "owner"), "owner request .unibus");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, serve, NULL), "start owner server");
    StyxClient *forward = styxclientmountphy(phybidirchannel(caller_mux, 0), 256, "caller", "", 3000, 4);
    DVM_Provider *request = forward ? styxmountproviderat(forward, ".unibus", FVM_ACCESS_WRITE) : NULL;
    check(request != NULL, "mount owner request file");
    check(styxscalarcallfile(request, 18, argument) != DVM_LAUNCH_DELIVERED, "wrong request tag");
    check(styxscalarcallfile(request, 17, argument) == DVM_LAUNCH_DELIVERED, "admitted scalar call");
    DVM_VfsHandle received = {0};
    check(dvm_provider_open(styxunibusserverinbox(receiver), ".", "r", &received) == FVM_OK, "open reply inbox");
    unsigned char frame[STYX_SCALAR_RECORD_CAPACITY];
    DVM_IoResult result = dvm_vfs_read_chunk(&received, frame, sizeof(frame));
    StyxScalarKind kind;
    int64_t value;
    check(result.status == FVM_OK && styxscalarread(29, frame, result.count, &kind, &value) &&
        kind == expected_kind && value == expected, "decode reverse scalar reply");
    dvm_vfs_close(&received);
    shutdown(breaker, SHUT_RDWR);
    check(!pthread_join(worker, NULL) && styxunibusserverjoin(receiver), "join both Styx roles");
    dvm_provider_release(request);
    dvm_provider_release(reply);
    phybidirfree(caller_mux);
    phybidirfree(owner_mux);
    close(breaker);
}


/* EN/RU: Exercise both reply types and verify wrong requests did not invoke the method.
 * Проверить оба вида ответа и отсутствие вызовов для чужого тега. */
int main(void)
{
    round_trip(123, STYX_SCALAR_VALUE, 124);
    round_trip(-7, STYX_SCALAR_ERROR, 42);
    check(calls == 2, "exact owner method count");
    puts("PASS scalar value and error over separate Styx Unibus routes");
    return 0;
}
