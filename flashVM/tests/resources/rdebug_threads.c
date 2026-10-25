/* Two actual native debugger clients never call a VM context; owner waits without polling. */
#include "flashvm/rdebug.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>

#define CLIENTS 2U
#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "debug threads:%d %s\n", __LINE__, #x);                                                    \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


typedef struct Host
{
    pthread_mutex_t queue_lock, wait_lock;
    pthread_cond_t changed;
    pthread_t owner;
    unsigned int ready, posted;
} Host;


typedef struct Client
{
    Host *host;
    FVM_RDispatch *dispatch;
    FVM_RHandle resource;
    FVM_RWatch watch;
    unsigned long session;
    int failed;
} Client;


static void lock_queue(void *p)
{
    pthread_mutex_lock(&((Host *)p)->queue_lock);
}


static void unlock_queue(void *p)
{
    pthread_mutex_unlock(&((Host *)p)->queue_lock);
}


static int is_owner(void *p)
{
    return pthread_equal(((Host *)p)->owner, pthread_self());
}


static int post(Client *c, unsigned long id, unsigned long operation, const void *bytes, size_t size)
{
    FVM_RRequest q;
    memset(&q, 0, sizeof(q));
    q.session = c->session;
    q.id = id;
    q.target = c->resource;
    q.operation = operation;
    q.data = bytes;
    q.size = size;
    return fvm_rdispatch_post(c->dispatch, &q) == FVM_R_OK;
}


static int reply(Client *c, unsigned long id)
{
    FVM_RReply result;
    FVM_RResult status;
    do
    {
        status = fvm_rdispatch_take(c->dispatch, c->session, id, &result);
        if (status == FVM_R_PENDING)
        {
            sched_yield();
        }
    } while (status == FVM_R_PENDING);
    return status == FVM_R_OK && result.result == FVM_R_OK;
}


static void *client(void *raw)
{
    Client *c = (Client *)raw;
    FVM_REvent event;
    FVM_RDebugSnapshot snapshot;
    FVM_SInteger integer;
    unsigned char bytes[FVM_SRECORD_INTEGER_MAX];
    size_t used;
    FVM_RResult status;
    pthread_mutex_lock(&c->host->wait_lock);
    while (!c->host->ready)
    {
        pthread_cond_wait(&c->host->changed, &c->host->wait_lock);
    }
    pthread_mutex_unlock(&c->host->wait_lock);
    if (fvm_rdispatch_event_take(c->dispatch, c->session, c->watch, &event) != FVM_R_OK ||
        event.kind != FVM_RDEBUG_STOP_EVENT || fvm_rdebug_snapshot_read(event.data, event.size, &snapshot) != FVM_R_OK)
    {
        c->failed = 1;
    }
    if (!c->failed && c->session == 1)
    {
        integer.high = 0;
        integer.low = snapshot.stop;
        integer.negative = 0;
        if (fvm_sinteger_write(&integer, bytes, sizeof(bytes), &used) != FVM_S_OK ||
            !post(c, 1, FVM_RDEBUG_CLAIM, 0, 0) || !post(c, 2, FVM_RDEBUG_RESUME, bytes, used))
        {
            c->failed = 1;
        }
    }
    else if (!c->failed && !post(c, 1, FVM_RDEBUG_SNAPSHOT, 0, 0))
    {
        c->failed = 1;
    }
    pthread_mutex_lock(&c->host->wait_lock);
    ++c->host->posted;
    pthread_cond_broadcast(&c->host->changed);
    pthread_mutex_unlock(&c->host->wait_lock);
    if (c->failed)
    {
        return 0;
    }
    if (!reply(c, 1) || (c->session == 1 && !reply(c, 2)))
    {
        c->failed = 1;
    }
    do
    {
        status = fvm_rdispatch_event_take(c->dispatch, c->session, c->watch, &event);
        if (status == FVM_R_PENDING)
        {
            sched_yield();
        }
    } while (status == FVM_R_PENDING);
    if (status != FVM_R_OK || event.kind != FVM_RDEBUG_RESUME_EVENT ||
        fvm_rdebug_snapshot_read(event.data, event.size, &snapshot) != FVM_R_OK || snapshot.state != FVM_RDEBUG_RUNNING)
    {
        c->failed = 1;
    }
    return 0;
}


static FVM_Result service(FVM_RDebug *debug, void *raw)
{
    Host *h = (Host *)raw;
    (void)debug;
    pthread_mutex_lock(&h->wait_lock);
    h->ready = 1;
    pthread_cond_broadcast(&h->changed);
    while (h->posted < CLIENTS)
    {
        pthread_cond_wait(&h->changed, &h->wait_lock);
    }
    pthread_mutex_unlock(&h->wait_lock);
    return FVM_OK;
}


int main(void)
{
    const unsigned char body[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    Host host;
    Client clients[CLIENTS];
    pthread_t workers[CLIENTS];
    FVM_RPolicy policy;
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_Object *function;
    FVM_RDispatch *dispatch;
    FVM_RDebug *debug;
    FVM_RHandle resource;
    FVM_Value result;
    unsigned int i;
    memset(&host, 0, sizeof(host));
    memset(&policy, 0, sizeof(policy));
    memset(clients, 0, sizeof(clients));
    CHECK(!pthread_mutex_init(&host.queue_lock, 0) && !pthread_mutex_init(&host.wait_lock, 0) &&
          !pthread_cond_init(&host.changed, 0));
    host.owner = pthread_self();
    policy.struct_size = sizeof(policy);
    policy.lock = lock_queue;
    policy.unlock = unlock_queue;
    policy.is_owner = is_owner;
    policy.userdata = &host;
    CHECK(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK);
    CHECK(fvm_function_create_avm1(context, body, sizeof(body), 0, 0, 0, &function) == FVM_OK &&
          fvm_gc_pin(context, function) == FVM_OK);
    CHECK(fvm_rdispatch_create(&policy, &dispatch) == FVM_R_OK &&
          fvm_rdebug_create(context, dispatch, "/debug/ctl", service, &host, &debug, &resource) == FVM_R_OK);
    for (i = 0; i < CLIENTS; ++i)
    {
        Client *c = clients + i;
        c->host = &host;
        c->dispatch = dispatch;
        c->resource = resource;
        c->session = (unsigned long)i + 1;
        CHECK(fvm_rdispatch_grant(
                  dispatch, resource, c->session, FVM_R_READ | FVM_R_SUBSCRIBE | (i ? 0 : FVM_R_CONTROL)) == FVM_R_OK);
        CHECK(fvm_rdispatch_subscribe(dispatch, c->session, resource, &c->watch) == FVM_R_OK &&
              !pthread_create(workers + i, 0, client, c));
    }
    CHECK(fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, debug) == FVM_OK &&
          fvm_debug_set_breakpoint(context, "<function>", 0, 1) == FVM_OK);
    CHECK(fvm_function_invoke(context, function, 0, 0, 0, &result) == FVM_OK && result.number == 42);
    for (i = 0; i < CLIENTS; ++i)
    {
        CHECK(!pthread_join(workers[i], 0) && !clients[i].failed);
    }
    CHECK(fvm_debug_set_breakpoint_handler(context, 0, 0) == FVM_OK && fvm_rdebug_destroy(&debug) == FVM_R_OK &&
          fvm_rdispatch_destroy(&dispatch) == FVM_R_OK);
    CHECK(fvm_gc_unpin(context, function) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    CHECK(!pthread_cond_destroy(&host.changed) && !pthread_mutex_destroy(&host.wait_lock) &&
          !pthread_mutex_destroy(&host.queue_lock));
    puts("PASS two debugger client threads, blocking owner service, independent events and "
         "owner-only VM access");
    return 0;
}
