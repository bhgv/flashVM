/* UI main thread and a real, independently bound Dis scheduler worker. */
#include "editor_thread.h"
#include "editor_view.h"
#include "editor_ink.h"
#include "disvm/native_context.h"
#include "managed_workers.h"
#include "thread_port.h"
#include "channel_sync.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* One process-wide host session; its VMs each retain one exclusive heap owner. */
typedef struct EditorThread
{
    FVM_PortSync *sync; /* Protect state publication and RPC request/reply. */
    FVM_PortThread *worker; /* Joined before any borrowed launch data is freed. */
    int (*entry)(void *); /* Real runtime entry, not a mock instruction callback. */
    void *user; /* Borrowed native launch arguments, never a Dis object. */
    unsigned int operation; /* Single Dis owner permits one outstanding RPC. */
    unsigned long token; /* Copied native ink identity. */
    char *input; /* Owned counted copy, no source heap address retained. */
    size_t size; /* Initialized input bytes. */
    char *output; /* Owned reply bytes handed to the Dis caller. */
    long scalar; /* Copied reply scalar. */
    int answered; /* UI published the reply under sync. */
    int reply_ok; /* One only for successful RPC execution. */
    int initialized; /* UI creation was attempted; main must destroy partial state. */
    int running; /* UI initialization succeeded; event polling is admitted. */
    int stopping; /* Dis has begun owner teardown. */
    int failed; /* Latched native UI/RPC failure. */
    int done; /* Worker published terminal status after TLS unbind. */
    int result; /* Runtime exit code. */
    int worker_bound; /* Real Dis entry returned with its own extended-up TLS. */
    int worker_released; /* That owner unbound/destroyed TLS before publishing done. */
    unsigned long polls; /* Main-loop iterations while the worker remains live. */
} EditorThread;

static EditorThread *session; /* Published before start, reset only after join. */


/* Query immutable lifecycle mode; caller is main or its live Dis worker. */
int dvm_editor_threaded(void)
{
    return session != NULL;
}


/* Copy a borrowed native string into independently owned reply storage. */
static char *copy_text(const char *text)
{
    size_t size = text ? strlen(text) : 0;
    char *copy = malloc(size+1);
    if (copy)
    {
        memcpy(copy,text ? text : "",size);
        copy[size] = 0;
    }
    return copy;
}


/* Execute one copied request solely on UI main. Called with sync held: no
 * callback may reenter Dis/RPC. No managed Dis value is read or written here. */
static int serve(EditorThread *s)
{
    switch (s->operation)
    {
    case DVM_EDITOR_RPC_START:
        s->initialized = 1;
        s->scalar = dvm_editor_default_font(s->input) && dvm_editor_create();
        s->running = s->scalar != 0;
        return s->scalar != 0;
    case DVM_EDITOR_RPC_ROWS:
        s->scalar = dvm_editor_rows();
        return 1;
    case DVM_EDITOR_RPC_LINES:
        s->scalar = dvm_editor_line_numbers();
        return 1;
    case DVM_EDITOR_RPC_FRESH:
        s->scalar = dvm_editor_fresh();
        return 1;
    case DVM_EDITOR_RPC_PATH:
        s->output = copy_text(dvm_editor_path());
        return s->output != NULL;
    case DVM_EDITOR_RPC_INK:
        s->output = copy_text(dvm_ink_read(s->token));
        return s->output != NULL;
    case DVM_EDITOR_RPC_ACK:
        dvm_ink_ack(s->token,s->input,s->size);
        return 1;
    case DVM_EDITOR_RPC_CANCEL:
        dvm_ink_cancel(s->token);
        return 1;
    default:
        return 0;
    }
}


/* Publish copied arguments then park the Dis owner, leaving UI main runnable.
 * A failure never returns a pointer into the UI heap. Input stays caller-owned. */
int dvm_editor_thread_rpc(unsigned int operation, unsigned long token,
    const char *input, size_t size, long *scalar, char **output)
{
    EditorThread *s = session;
    char *copy;
    int ok;
    if (!s || (size && !input) || size > DVM_EDITOR_RPC_BYTES ||
        (!fvm_port_thread_is_current(s->worker) && !dvm_managed_workers_current()))
    {
        return 0;
    }
    if (output)
    {
        *output = NULL;
    }
    copy = malloc(size+1);
    if (!copy)
    {
        return 0;
    }
    if (size)
    {
        memcpy(copy,input,size);
    }
    copy[size] = 0;
    fvm_port_sync_lock(s->sync);
    if (s->failed || s->stopping || s->operation)
    {
        fvm_port_sync_unlock(s->sync);
        free(copy);
        return 0;
    }
    s->input = copy;
    s->size = size;
    s->token = token;
    s->operation = operation;
    s->answered = 0;
    s->reply_ok = 0;
    while (!s->answered && !s->failed)
    {
        fvm_port_sync_wait(s->sync);
    }
    ok = s->answered && s->reply_ok;
    if (ok && scalar)
    {
        *scalar = s->scalar;
    }
    if (ok && output)
    {
        *output = s->output;
        s->output = NULL;
    }
    free(s->output);
    s->output = NULL;
    s->operation = 0;
    s->input = NULL;
    fvm_port_sync_unlock(s->sync);
    free(copy);
    return ok;
}


/* Publish shutdown before queue close so main distinguishes expected EOF. */
void dvm_editor_thread_stopping(void)
{
    EditorThread *s = session;
    if (s)
    {
        fvm_port_sync_lock(s->sync);
        s->stopping = 1;
        fvm_port_sync_signal_all(s->sync);
        fvm_port_sync_unlock(s->sync);
    }
}


/* Scheduler-boundary observation; no Dis heap or UI callback is involved. */
int dvm_editor_thread_failed(void)
{
    EditorThread *s = session;
    int failed = 0;
    if (s)
    {
        fvm_port_sync_lock(s->sync);
        failed = s->failed;
        fvm_port_sync_unlock(s->sync);
    }
    return failed;
}


/* Recheck a close raced with an earlier main-loop state observation. */
int dvm_editor_thread_is_stopping(void)
{
    EditorThread *s = session;
    int stopping = 0;
    if (s)
    {
        fvm_port_sync_lock(s->sync);
        stopping = s->stopping;
        fvm_port_sync_unlock(s->sync);
    }
    return stopping;
}


/* Run the entire runtime on this OS owner and retire its extended-up TLS.
 * The runtime's managed state never migrates back to main during cleanup. */
static void execute(void *argument)
{
    EditorThread *s = argument;
    DVM_NativeContext *context;
    int result;
    /* Wait for the launcher to publish the native handle before RPC can check it. */
    fvm_port_sync_lock(s->sync);
    while (!s->worker)
    {
        fvm_port_sync_wait(s->sync);
    }
    fvm_port_sync_unlock(s->sync);
    if (!fvm_port_thread_is_current(s->worker) || dvm_native_context_current())
    {
        fvm_port_sync_lock(s->sync);
        s->result = 1;
        s->done = 1;
        fvm_port_sync_unlock(s->sync);
        return;
    }
    result = s->entry(s->user);
    {
        extern void dvm_editor_stop(void);
        dvm_editor_stop();
    }
    context = dvm_native_context_current();
    s->worker_bound = context != NULL;
    if (context && (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context)))
    {
        result = 1;
    }
    s->worker_released = dvm_native_context_current() == NULL;
    fvm_port_sync_lock(s->sync);
    s->result = result;
    s->done = 1;
    fvm_port_sync_signal_all(s->sync);
    fvm_port_sync_unlock(s->sync);
}


/* Drive UI/SDL on main independently of Dis instruction quanta. All RPC and
 * terminal predicates are synchronized; joins run without holding the mutex. */
int dvm_editor_thread_launch(int (*entry)(void *), void *user)
{
    EditorThread state;
    int result;
    if (!entry || session || dvm_native_context_current())
    {
        return 1;
    }
    memset(&state,0,sizeof(state));
    state.entry = entry;
    state.user = user;
    state.sync = fvm_port_sync_create();
    if (!state.sync || !dvm_editor_transport_prepare())
    {
        if (state.sync)
        {
            fvm_port_sync_destroy(state.sync);
        }
        dvm_editor_transport_finish();
        return 1;
    }
    session = &state;
    fvm_port_sync_lock(state.sync);
    state.worker = fvm_port_thread_start(execute,&state);
    fvm_port_sync_signal_all(state.sync);
    fvm_port_sync_unlock(state.sync);
    if (!state.worker)
    {
        session = NULL;
        dvm_editor_transport_finish();
        fvm_port_sync_destroy(state.sync);
        return 1;
    }
    for (;;)
    {
        int done;
        int stopping;
        int active;
        int progress = 0;
        fvm_port_sync_lock(state.sync);
        if (state.operation && !state.answered && !state.failed)
        {
            state.reply_ok = serve(&state);
            state.answered = 1;
            fvm_port_sync_signal_all(state.sync);
        }
        done = state.done;
        stopping = state.stopping;
        active = state.running;
        state.polls++;
        fvm_port_sync_unlock(state.sync);
        if (active)
        {
            progress = dvm_editor_ui_poll(stopping);
            if (progress < 0)
            {
                fvm_port_sync_lock(state.sync);
                state.failed = 1;
                fvm_port_sync_signal_all(state.sync);
                fvm_port_sync_unlock(state.sync);
                dvm_ui_close(dvm_editor_transport());
            }
        }
        if (done)
        {
            break;
        }
        if (!progress)
        {
            fvm_port_thread_sleep_us(1000);
        }
    }
    fvm_port_thread_join(state.worker);
    result = state.failed ? 1 : state.result;
    if (getenv("DVM_EDITOR_THREAD_STATS"))
    {
        printf("NATIVEOWNERS ui=main dis=worker ui_polls=%lu main_dis_tls=%d worker_bound=%d worker_released=%d result=%d\n",
            state.polls,dvm_native_context_current() != NULL,state.worker_bound,state.worker_released,result);
    }
    if (state.initialized)
    {
        dvm_editor_destroy();
    }
    dvm_editor_transport_finish();
    session = NULL;
    fvm_port_sync_destroy(state.sync);
    return result;
}


/* Ordinary presentation records and bulk egress use the same bounded copied
 * UI queue, preserving acceptance order without exposing a managed array. */
int dvm_editor_deliver(const DVM_UIMessage *message)
{
    if (!session)
    {
        return dvm_editor_present(message);
    }
    return dvm_ui_send(dvm_editor_transport(),DVM_UI_DIS,message,1) == FVM_OK;
}
