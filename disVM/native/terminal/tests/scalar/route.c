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
#include "scalar_function.h"
#include "scalar_interface.h"
#include "scalar_gateway.h"
#include "scalar_owner.h"
#include "unibus_server.h"
#include "mount.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef STYX_SCALAR_GUEST
#define REQUEST_TAG 4294967295UL /* Exercise the maximum unsigned tag from Limbo. */
#define REPLY_TAG 64UL /* The first tag needing a leading zero magnitude group. */
#else
#define REQUEST_TAG 17UL /* Native fixture request tag. */
#define REPLY_TAG 29UL /* Native fixture reply tag. */
#endif

static Styxserver owner; /* One fixture owner thread dispatches the method. */
static DVM_Provider *reply; /* Retained reverse file while owner serves requests. */
#ifdef STYX_SCALAR_GUEST
extern void scalar_guest_mounted(DVM_Provider *); /* Runs a guest on a plain mount. */
extern void scalar_guest(DVM_Provider *, DVM_Provider *); /* Runtime test entry. */
#endif
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


#ifndef STYX_SCALAR_GUEST
static DVM_Provider *dispatch_inbox; /* Immutable during the native server thread. */


/* EN/RU: Queue a request without executing the owner method on the Styx thread.
 * Поставить запрос в очередь без вызова метода на потоке Styx. */
static char *queue_call(Qid qid, char *bytes, ulong *count, vlong offset)
{
    DVM_VfsHandle writer = {0};
    if (qid.path != 1 || offset != 0 ||
        dvm_provider_open(dispatch_inbox, ".", "wo", &writer) != FVM_OK)
    {
        return Ebadarg;
    }
    DVM_IoResult got = dvm_vfs_write_chunk(&writer, bytes, *count);
    dvm_vfs_close(&writer);
    return got.status == FVM_OK && got.count == *count ? nil : Ebadarg;
}
#endif


#ifdef STYX_SCALAR_GUEST
/* EN/RU: Validate the request and send a separate S-record through the reverse session.
 * Проверить запрос и отправить отдельную S-запись через встречную сессию. */
static char *receive_call(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (getenv("DVM_TEST_SCALAR_DROP_REPLY"))
    {
        StyxScalarKind kind;
        int64_t argument;
        int64_t result;
        if (qid.path == 1 && offset == 0 &&
            styxscalarread(REQUEST_TAG, bytes, *count, &kind, &argument) && kind == STYX_SCALAR_CALL)
        {
            method(NULL, argument, &result);
            return nil;
        }
        return Ebadarg;
    }
    return qid.path == 1 && offset == 0 &&
        styxscalarreplyfile(reply, REQUEST_TAG, bytes, *count, REPLY_TAG, method, NULL) == DVM_LAUNCH_DELIVERED ? nil : Ebadarg;
}

#endif

#ifndef STYX_SCALAR_GUEST
static Styxops operations = {.write = queue_call};
#else
static Styxops operations = {.write = receive_call};
#endif


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
static void round_trip(void (*exercise)(DVM_Provider *, DVM_Provider *))
{
    int pair[2];
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "connected socketpair");
    int breaker = dup(pair[0]);
    check(breaker >= 0, "shutdown descriptor");
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    PhyBidir *caller_mux = phybidircreate(phyposixfd(pair[0], caps), 1024);
    PhyBidir *owner_mux = phybidircreate(phyposixfd(pair[1], caps), 1024);
    check(caller_mux && owner_mux, "bidirectional carrier");
    StyxUnibusServer *receiver = styxunibusserverstart(phybidirchannel(caller_mux, 1), REPLY_TAG);
    check(receiver != NULL, "caller reverse .unibus");
    StyxClient *reverse = styxclientmountphy(phybidirchannel(owner_mux, 1), 256, "owner", "", 3000, 4);
    reply = reverse ? styxmountproviderat(reverse, ".unibus", FVM_ACCESS_WRITE) : NULL;
    check(reply != NULL, "mount caller reply file");
    memset(&owner, 0, sizeof(owner));
    check(!styxphyinit(&owner, &operations, phybidirchannel(owner_mux, 0), 0555, 1, 2) &&
        styxaddfile(&owner, Qroot, 1, ".unibus", 0222, "owner"), "owner request .unibus");
#ifndef STYX_SCALAR_GUEST
    dispatch_inbox = styxunibusinbox(REQUEST_TAG);
    check(dispatch_inbox != NULL, "owner inbox");
#endif
    pthread_t worker;
    check(!pthread_create(&worker, NULL, serve, NULL), "start owner server");
    StyxClient *forward = styxclientmountphy(phybidirchannel(caller_mux, 0), 256, "caller", "", 3000, 4);
    DVM_Provider *request = forward ? styxmountproviderat(forward, ".unibus", FVM_ACCESS_WRITE) : NULL;
    check(request != NULL, "mount owner request file");
    exercise(request, styxunibusserverinbox(receiver));
    shutdown(breaker, SHUT_RDWR);
    check(!pthread_join(worker, NULL) && styxunibusserverjoin(receiver), "join both Styx roles");
#ifndef STYX_SCALAR_GUEST
    dvm_provider_release(dispatch_inbox);
#endif
    dvm_provider_release(request);
    dvm_provider_release(reply);
    phybidirfree(caller_mux);
    phybidirfree(owner_mux);
    close(breaker);
}


#ifndef STYX_SCALAR_GUEST

/* EN/RU: Exchange one value or method error over the supplied route.
 * Обменять значение или ошибку через переданную пару маршрутов. */
static void exchange(DVM_Provider *request, DVM_Provider *inbox, int64_t argument,
    StyxScalarKind expected_kind, int64_t expected)
{
    check(styxscalarcallfile(request, 18, argument) != DVM_LAUNCH_DELIVERED, "wrong request tag");
    const uint32_t signature = UINT32_C(0xf1234567); /* Fixture compiled signature bits. */
    DVM_Provider *profile = styxunibusinbox(61);
    check(profile && styxscalaroffer(profile, 61, "compute", signature) == DVM_LAUNCH_DELIVERED,
        "publish exact interface file");
    check(styxscalargateway(profile, 61, "compute", signature ^ 1U, request, REQUEST_TAG,
        inbox, REPLY_TAG) == NULL, "interface mismatch refuses gateway");
    StyxScalarGateway *gateway = styxscalargateway(profile, 61, "compute", signature, request,
        REQUEST_TAG, inbox, REPLY_TAG);
    dvm_provider_release(profile);
    check(gateway != NULL, "interface admits gateway");
    StyxScalarOwner *owner = styxscalarowner(dispatch_inbox, REQUEST_TAG, reply, REPLY_TAG);
    int64_t input = 0;
    check(owner && styxscalarownertake(owner, &input) == STYX_GATEWAY_PENDING && input == 0,
        "owner inbox pending");
    int64_t value = 99;
    check(styxscalargatewayreceive(gateway, &value) == STYX_GATEWAY_UNKNOWN && value == 99,
        "no reply before submission");
    check(styxscalargatewaysend(gateway, argument) == DVM_LAUNCH_DELIVERED, "admitted scalar call");
    check(styxscalargatewaysend(gateway, argument) == DVM_LAUNCH_NOT_SENT, "gateway never resends");
    check(styxscalargatewayreceive(gateway, &value) == STYX_GATEWAY_PENDING && value == 99,
        "Styx acknowledgement precedes method execution");
    check(styxscalarownertake(owner, &input) == STYX_GATEWAY_VALUE && input == argument &&
        styxscalarownertake(owner, &input) == STYX_GATEWAY_UNKNOWN, "admit request exactly once");
    int64_t result;
    int success = method(NULL, input, &result);
    check(styxscalarownersend(owner, success, result) == DVM_LAUNCH_DELIVERED &&
        styxscalarownersend(owner, success, result) == DVM_LAUNCH_NOT_SENT, "send reply once");
    styxscalarownerfree(owner);
    StyxScalarOutcome outcome = expected_kind == STYX_SCALAR_VALUE ? STYX_GATEWAY_VALUE : STYX_GATEWAY_ERROR;
    check(styxscalargatewayreceive(gateway, &value) == outcome && value == expected, "decode reverse scalar reply");
    check(styxscalargatewayreceive(gateway, &value) == STYX_GATEWAY_UNKNOWN && value == expected,
        "reply consumed exactly once");
    styxscalargatewayfree(gateway);
}


/* EN/RU: Validate the success case with an independent native client.
 * Проверить результат независимым нативным клиентом. */
static void native_value(DVM_Provider *request, DVM_Provider *inbox)
{
    exchange(request, inbox, 123, STYX_SCALAR_VALUE, 124);
}


/* EN/RU: Validate explicit owner refusal with an independent native client.
 * Проверить ошибку владельца независимым нативным клиентом. */
static void native_error(DVM_Provider *request, DVM_Provider *inbox)
{
    exchange(request, inbox, -7, STYX_SCALAR_ERROR, 42);
}


/* EN/RU: Admit a compiled import through the actual reverse Styx file.
 * Допустить компилированный импорт через встречный файл Styx. */
static void native_interface(DVM_Provider *request, DVM_Provider *inbox)
{
    const uint32_t signature = UINT32_C(0xf1234567); /* Exact unsigned compiled signature bits. */
    const char *name = "compute"; /* Fixture export name. */
    check(styxscalaroffer(reply, REPLY_TAG, name, signature) == DVM_LAUNCH_DELIVERED, "deliver interface");
    DVM_VfsHandle received = {0};
    check(dvm_provider_open(inbox, ".", "r", &received) == FVM_OK, "open interface inbox");
    unsigned char bytes[STYX_SCALAR_INTERFACE_CAPACITY];
    DVM_IoResult result = dvm_vfs_read_chunk(&received, bytes, sizeof(bytes));
    check(result.status == FVM_OK && styxscalaraccept(REPLY_TAG, bytes, result.count, name, signature),
        "match compiled import");
    check(!styxscalaraccept(REPLY_TAG, bytes, result.count, name, signature ^ 1U) &&
        !styxscalaraccept(REPLY_TAG, bytes, result.count, "other", signature) &&
        !styxscalaraccept(REPLY_TAG, bytes, result.count - 1, name, signature) &&
        !styxscalaraccept(REPLY_TAG + 1, bytes, result.count, name, signature), "reject mismatches");
    check(styxscalarcallchecked(request, REQUEST_TAG, REPLY_TAG, bytes, result.count,
        name, signature ^ 1U, 123) == DVM_LAUNCH_NOT_SENT, "mismatch prevents submission");
    dvm_vfs_close(&received);
}


#endif

#ifdef STYX_SCALAR_GUEST
/* EN: Check the wire PID against the guest's independent observation.
 * RU: Сверить PID из запроса с PID, полученным внутри гостя. */
static int caller_method(void *context, uint32_t pid, int64_t argument, int64_t *result)
{
    static uint32_t previous;
    (void)context;
    if (argument != (int64_t)pid || pid == previous)
        return 0;
    previous = pid;
    ++calls;
    *result = pid;
    return 1;
}
#endif

/* EN/RU: Interleave two live fids and read their replies in reverse order.
 * Чередовать операции двух открытых fid и прочитать ответы в обратном порядке. */
static void mounted_function(void)
{
    int pair[2];
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "mounted function carrier");
    int breaker = dup(pair[0]);
    check(breaker >= 0, "mounted function shutdown descriptor");
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    StyxScalarFunction function = {STYX_FUNCTION_REQUEST_TAG, STYX_FUNCTION_REPLY_TAG, method, NULL, NULL};
#ifdef STYX_SCALAR_GUEST
    if (getenv("DVM_TEST_SCALAR_CALLER"))
    {
        function.request_tag = STYX_FUNCTION_CALLER_TAG;
        function.method = NULL;
        function.caller_method = caller_method;
    }
#endif
    StyxUnibusServer *server = styxunibusserverstartfunction(phyposixfd(pair[1], caps),
        "compute.dis", "compute", &function);
    check(server != NULL, "export mounted function");
    StyxClient *client = styxclientmountphy(phyposixfd(pair[0], caps), 256, "caller", "", 3000, 4);
    check(client != NULL, "mount function server");
#ifdef STYX_SCALAR_GUEST
    DVM_Provider *mount = styxmountprovider(client, FVM_ACCESS_READ | FVM_ACCESS_WRITE | FVM_ACCESS_EXEC);
    check(mount != NULL, "ordinary Styx mount");
    scalar_guest_mounted(mount);
    dvm_provider_release(mount);
#else
    StyxFid *profile = styxclientwalk(client, "mounted/compute.dis/.interface");
    unsigned char offer[STYX_SCALAR_INTERFACE_CAPACITY];
    StyxTransfer profile_read;
    check(profile && styxclientopen(profile, OREAD), "readable mounted profile");
    styxclientread(profile, 0, offer, sizeof(offer), &profile_read);
    check(profile_read.success && styxscalaraccept(STYX_FUNCTION_PROFILE_TAG, offer,
        profile_read.count, "compute", DVM_SCALAR_INT_SIGNATURE), "compiled mounted profile");
    styxclientclunk(profile);
    StyxFid *first = styxclientwalk(client, "mounted/compute.dis/compute");
    StyxFid *second = styxclientwalk(client, "mounted/compute.dis/compute");
    check(first && second && styxclientopen(first, ORDWR) && styxclientopen(second, ORDWR),
        "two opened fids for the same function");
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t size;
    StyxTransfer transfer;
    StyxFid *rejected = styxclientwalk(client, "mounted/compute.dis/compute");
    check(rejected && styxclientopen(rejected, ORDWR), "open malformed request fid");
    check(styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL, 70, bytes, sizeof(bytes), &size), "encode value");
    styxclientwrite(rejected, 0, bytes, size - 1, &transfer);
    check(!transfer.success, "truncated request refused");
    styxclientclunk(rejected);
    rejected = styxclientwalk(client, "mounted/compute.dis/compute");
    check(rejected && styxclientopen(rejected, ORDWR), "open out-of-range request fid");
    check(styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL,
        (int64_t)INT32_MAX + 1, bytes, sizeof(bytes), &size), "encode wide argument");
    styxclientwrite(rejected, 0, bytes, size, &transfer);
    check(!transfer.success, "wide argument refused by the typed server");
    styxclientclunk(rejected);
    check(styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL, 70, bytes, sizeof(bytes), &size),
        "restore valid request");
    styxclientwrite(first, 0, bytes, size, &transfer);
    check(transfer.success && transfer.confirmed && transfer.count == size, "first invocation");
    check(styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL, -1, bytes, sizeof(bytes), &size), "encode error case");
    styxclientwrite(second, 0, bytes, size, &transfer);
    check(transfer.success && transfer.count == size, "second invocation");
    StyxScalarKind kind;
    int64_t value;
    styxclientread(second, 0, bytes, sizeof(bytes), &transfer);
    check(transfer.success && styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, transfer.count, &kind, &value) &&
        kind == STYX_SCALAR_ERROR && value == 42, "second fid retains its own error");
    styxclientclunk(second);
    styxclientread(first, 0, bytes, 1, &transfer);
    check(transfer.success && transfer.count == 1, "first reply prefix");
    styxclientread(first, 1, bytes + 1, sizeof(bytes) - 1, &transfer);
    check(transfer.success && styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, transfer.count + 1, &kind, &value) &&
        kind == STYX_SCALAR_VALUE && value == 71, "first fid retains its own value");
    check(styxscalarwrite(STYX_FUNCTION_REQUEST_TAG, STYX_SCALAR_CALL, 70, bytes, sizeof(bytes), &size), "encode duplicate");
    styxclientwrite(first, 0, bytes, size, &transfer);
    check(!transfer.success, "second write refused on same fid");
    styxclientclunk(first);
#endif
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxclientfree(client);
    check(styxunibusserverjoin(server), "join mounted function server");
}


#ifndef STYX_SCALAR_GUEST
/* EN/RU: Exercise both reply types and verify wrong requests did not invoke the method.
 * Проверить оба вида ответа и отсутствие вызовов для чужого тега. */
int main(void)
{
    round_trip(native_interface);
    round_trip(native_value);
    round_trip(native_error);
    check(calls == 2, "exact owner method count");
    mounted_function();
    check(calls == 4, "exact method count after interleaved fids");
    puts("PASS mounted scalar function with two Styx fids");
    puts("PASS scalar value and error over separate Styx Unibus routes");
    return 0;
}
#else

/* EN/RU: Run one embedded guest, with exactly one accepted invocation.
 * Запустить одного встроенного гостя с одним принятым вызовом. */
int scalar_round_trip(void)
{
    if (getenv("DVM_TEST_SCALAR_MOUNTED"))
    {
        mounted_function();
        check(calls == (getenv("DVM_TEST_SCALAR_CALLER") ? 2 : 3),
            "exact typed call count at the mounted function");
        return 0;
    }
    round_trip(scalar_guest);
    check(calls == 1, "one guest call at owner");
    return 0;
}
#endif
