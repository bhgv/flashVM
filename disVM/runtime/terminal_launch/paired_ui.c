/* Copyright (c) 2026 bhgv
 * EN: Attach a Dis job's rooted local channels to the existing libstyxrpc session.
 * RU: Подключить удерживаемые локальные каналы задания Dis к существующей сессии libstyxrpc.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "kernel.h"
#include "disvm/unibus_binding.h"
#include "disvm/refcount.h"
#include "disvm/ui_frame.h"
#include "paired_ui.h"
#include "paired_exchange.h"
#include "styxrpc/unibus_terminal.h"
#include "root_registry.h"
#include "runtime_gc.h"
#include "sys_capture.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct PairedUi
{
    struct StyxClient *client;                /* Borrowed through mount teardown. */
    char *document;                           /* Owned startup path or NULL for a blank pane. */
    int fresh;                                /* Skip reading an existing file for --new. */
    pthread_mutex_t lock;                    /* Serializes builtin registration and controller I/O. */
    DvmPairedExchange exchange;              /* Mixed-message leases of the guest root. */
    StyxRpcTerminal terminal;                /* Native mailbox; no managed addresses in the transport. */
    Channel *input;                          /* [GC:] Explicitly held and registered. */
    Channel *output;                         /* [GC:] Explicitly held and registered. */
    unsigned char queued[STYXRPC_TERMINAL_MAX]; /* Exact copied outbound frame awaiting credit. */
    size_t queued_size;                      /* Pending outbound bytes. */
    int attached;                            /* Published to the controller with release ordering. */
    int stopping;                            /* Owner requested detach or its binding retired. */
} PairedUi;

typedef struct Transfer
{
    Channel *channel;        /* [GC:] Borrowed pinned channel on the controller. */
    Array *array;            /* [GC:] Temporary typed send source or receive destination. */
    int receive;             /* Ordinary ALT receive when set, send otherwise. */
    int matched;             /* Whether the native scheduler committed a transfer. */
    const void *bytes;       /* Borrowed incoming bytes when sending. */
    size_t size;             /* Incoming bytes; at most the terminal bound. */
} Transfer;

static PairedUi *current_ui; /* One mounted runtime, set before and reset after its controller. */

extern int dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive);


/* [GC:] EN: Pin a live pointer-mover channel with independent counted and GC ownership.
 * RU: Удержать живой канал с pointer mover отдельной ссылкой и корнем GC. */
static int retain_channel(Channel *channel)
{
    Heap *heap;
    if (!channel || channel == H || channel->mover != movp)
    {
        return 0;
    }
    heap = D2H(channel);
    if (!dvm_runtime_root_add(heap))
    {
        return 0;
    }
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        dvm_runtime_root_remove(heap);
        return 0;
    }
    Setmark(heap);
    return 1;
}


/* [GC:] EN: Retire one counted root on an admitted native heap owner.
 * RU: Снять один корень и ссылку в допущенном потоке владельца кучи. */
static void release_channel(Channel *channel)
{
    if (channel)
    {
        dvm_runtime_root_remove(D2H(channel));
        destroy(channel);
    }
}


/* EN: Clunk outside managed admission when available, then clear the copied state.
 * RU: Закрыть fid вне допуска к куче, когда возможно, и очистить копии. */
static void drop_port(PairedUi *ui)
{
    int released = dvm_runtime_gc_native_try_leave();
    styxrpc_messages_close(ui->terminal.port);
    if (released >= 0)
    {
        dvm_runtime_gc_native_enter(released);
    }
    memset(&ui->terminal, 0, sizeof(ui->terminal));
    ui->queued_size = 0;
}


/* [GC:] EN: Consume the channel pins and exact port while the caller holds the adapter lock.
 * RU: Освободить корни каналов и точный порт с удержанием мьютекса адаптера. */
static void detach_locked(PairedUi *ui)
{
    __atomic_store_n(&ui->attached, 0, __ATOMIC_RELEASE);
    release_channel(ui->input);
    release_channel(ui->output);
    ui->input = ui->output = NULL;
    if (ui->terminal.port)
    {
        drop_port(ui);
    }
}


/* [GC:] EN: Root both ports before leaving the heap to open the terminal on the existing mount.
 * RU: Удержать оба канала до выхода из кучи для открытия терминала существующего маунта. */
static int attach(PairedUi *ui, Channel *input, Channel *output)
{
    int result = -1;
    if (input == output || pthread_mutex_trylock(&ui->lock))
    {
        return result;
    }
    if (!ui->terminal.port && !__atomic_load_n(&ui->stopping, __ATOMIC_ACQUIRE) && retain_channel(input))
    {
        if (retain_channel(output))
        {
            StyxRpcMessageType types[2];
            StyxRpcMessageSpec spec = {0};
            styxrpc_terminal_types(&ui->terminal, types);
            spec.types = types;
            spec.count = 2;
            int released = dvm_runtime_gc_native_try_leave();
            StyxRpcStatus opened = released < 0 ? STYXRPC_INVALID :
                styxrpc_messages_open_at(ui->client, NULL, &spec, &ui->terminal.port);
            if (opened == STYXRPC_OK &&
                (styxrpc_terminal_allow(&ui->terminal) != STYXRPC_OK ||
                 styxrpc_messages_pump(ui->terminal.port) == STYXRPC_INVALID ||
                 !ui->terminal.writable))
            {
                styxrpc_messages_close(ui->terminal.port);
                memset(&ui->terminal, 0, sizeof(ui->terminal));
                opened = STYXRPC_INVALID;
            }
            if (released >= 0)
            {
                dvm_runtime_gc_native_enter(released);
            }
            if (opened == STYXRPC_OK)
            {
                ui->input = input;
                ui->output = output;
                __atomic_store_n(&ui->attached, 1, __ATOMIC_RELEASE);
                result = 0;
            }
            else
            {
                release_channel(output);
            }
        }
        if (result)
        {
            release_channel(input);
        }
    }
    pthread_mutex_unlock(&ui->lock);
    return result;
}


/* EN: Set an atomic stop request without entering the VM or waiting for a transfer.
 * RU: Запросить остановку атомарно, без входа в VM и ожидания передачи. */
static void retire(void *state)
{
    PairedUi *ui = state;
    __atomic_store_n(&ui->stopping, 1, __ATOMIC_RELEASE);
}


/* EN: Expose only implemented launch configuration and ordinary channel operations.
 * RU: Обслужить доступную конфигурацию запуска и операции обычных каналов. */
static int invoke(void *state, int pid, unsigned method, void *frame)
{
    PairedUi *ui = state;
    (void)pid;
    switch (method)
    {
    case UNIBUS_ATTACH:
    {
        F_Unibus_attach call;
        WORD result;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_attach, ret), sizeof(*call.ret));
        result = attach(ui, call.input, call.output);
        dvm_sys_scalar_result(&result, call.ret, sizeof(result));
        return 1;
    }
    case UNIBUS_DETACH:
        if (__atomic_load_n(&ui->attached, __ATOMIC_ACQUIRE))
        {
            retire(ui);
        }
        return 1;
    case UNIBUS_PATH:
    {
        F_Unibus_path call;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_path, ret), sizeof(*call.ret));
        retstr(ui->document ? ui->document : "", call.ret);
        return 1;
    }
    case UNIBUS_FRESH:
    {
        F_Unibus_fresh call;
        WORD value = ui->fresh;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_fresh, ret), sizeof(*call.ret));
        dvm_sys_scalar_result(&value, call.ret, sizeof(value));
        return 1;
    }
    case UNIBUS_LINES:
    {
        F_Unibus_linenumbers call;
        WORD value = 0;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_linenumbers, ret), sizeof(*call.ret));
        dvm_sys_scalar_result(&value, call.ret, sizeof(value));
        return 1;
    }
    default:
        return dvm_paired_exchange_invoke(&ui->exchange, ui->client, method, frame);
    }
}


/* EN: Release native-only storage after the runtime has retired all roots and port state.
 * RU: Освободить нативное хранилище после удаления корней и состояния порта. */
static void dispose(void *state)
{
    PairedUi *ui = state;
    dvm_paired_exchange_finish(&ui->exchange);
    pthread_mutex_destroy(&ui->lock);
    free(ui->document);
    free(ui);
}


/* EN/RU: See paired_ui.h. / См. paired_ui.h. */
UnibusBinding *dvm_paired_ui_begin(struct StyxClient *client, const char *document, int fresh)
{
    static const UnibusOps ops = {invoke, retire, dispose};
    PairedUi *ui = current_ui || !client ? NULL : calloc(1, sizeof(*ui));
    if (!ui)
    {
        return NULL;
    }
    if (pthread_mutex_init(&ui->lock, NULL))
    {
        free(ui);
        return NULL;
    }
    ui->client = client;
    if (document && !(ui->document = strdup(document)))
    {
        dispose(ui);
        return NULL;
    }
    ui->fresh = fresh;
    UnibusBinding *binding = unibusbinding(&ops, ui, UNIBUS_INHERIT | UNIBUS_OWNER_PORTS | UNIBUS_CLOSE_WITH_OWNER);
    if (!binding)
    {
        dispose(ui);
        return NULL;
    }
    current_ui = ui;
    return binding;
}


/* EN/RU: See paired_ui.h. / См. paired_ui.h. */
void dvm_paired_ui_end(UnibusBinding *binding)
{
    if (binding)
    {
        current_ui = NULL;
        unibusrelease(binding);
    }
}


/* EN/RU: See paired_ui.h. / См. paired_ui.h. */
int dvm_paired_ui_active(void)
{
    return current_ui && __atomic_load_n(&current_ui->attached, __ATOMIC_ACQUIRE);
}


/* [GC:] EN: Prepare or receive a native byte-array channel transfer inside kprotect.
 * RU: Подготовить либо принять массив байтов канала внутри kprotect. */
static int transfer(void *context)
{
    Transfer *attempt = context;
    if (!attempt->receive)
    {
        attempt->array = H2D(Array *, heaparray(&Tbyte, (int)attempt->size));
        memmove(attempt->array->data, attempt->bytes, attempt->size);
    }
    attempt->matched = dvm_runtime_alt_try_transfer(attempt->channel, &attempt->array, attempt->receive);
    return 1;
}


/* [GC:] EN: Copy a checked envelope between the native mailbox and its original managed ports.
 * RU: Скопировать проверенный кадр между нативной очередью и исходными managed-каналами. */
static int move_local(PairedUi *ui)
{
    DVM_UIMessage validated;
    if (ui->terminal.size)
    {
        if (dvm_ui_frame_decode(ui->terminal.bytes, ui->terminal.size, &validated) != FVM_OK)
        {
            return 0;
        }
        Transfer attempt = {.channel = ui->input, .array = H,
            .bytes = ui->terminal.bytes, .size = ui->terminal.size};
        REG saved = DVM_R;
        int ok = kprotect(transfer, &attempt);
        DVM_R = saved;
        if (attempt.array != H && attempt.array)
        {
            destroy(attempt.array);
        }
        if (!ok)
        {
            return 0;
        }
        if (attempt.matched)
        {
            ui->terminal.size = 0;
        }
    }
    if (!ui->queued_size && ui->terminal.writable)
    {
        Transfer attempt = {.channel = ui->output, .array = H, .receive = 1};
        REG saved = DVM_R;
        int ok = kprotect(transfer, &attempt);
        DVM_R = saved;
        Array *array = attempt.array;
        if (ok && attempt.matched)
        {
            ok = array && array != H && array->t && array->t != H &&
                array->t->size == 1 && !array->t->np && array->len >= 0 &&
                (size_t)array->len <= sizeof(ui->queued) &&
                dvm_ui_frame_decode(array->data, (size_t)array->len, &validated) == FVM_OK;
            if (ok)
            {
                ui->queued_size = (size_t)array->len;
                memcpy(ui->queued, array->data, ui->queued_size);
            }
        }
        if (array && array != H)
        {
            destroy(array);
        }
        if (!ok)
        {
            return 0;
        }
    }
    return 1;
}


/* [ASINC] EN/RU: See paired_ui.h. / См. paired_ui.h. */
int dvm_paired_ui_poll(void)
{
    PairedUi *ui = current_ui;
    if (!dvm_paired_ui_active() || pthread_mutex_trylock(&ui->lock))
    {
        return 1;
    }
    int good = 1;
    if (__atomic_exchange_n(&ui->stopping, 0, __ATOMIC_ACQ_REL))
    {
        detach_locked(ui);
    }
    else
    {
        good = move_local(ui) && styxrpc_terminal_allow(&ui->terminal) != STYXRPC_INVALID;
        if (good && ui->queued_size)
        {
            StyxRpcStatus status = styxrpc_terminal_send(&ui->terminal, ui->queued, ui->queued_size);
            if (status == STYXRPC_OK)
            {
                ui->queued_size = 0;
            }
            good = status != STYXRPC_INVALID;
        }
        if (good)
        {
            int released = dvm_runtime_gc_native_try_leave();
            if (released >= 0)
            {
                good = styxrpc_messages_pump(ui->terminal.port) != STYXRPC_INVALID;
                dvm_runtime_gc_native_enter(released);
            }
        }
        if (!good)
        {
            detach_locked(ui);
        }
    }
    pthread_mutex_unlock(&ui->lock);
    return good;
}


/* EN/RU: See paired_ui.h. / См. paired_ui.h. */
void dvm_paired_ui_finish(void)
{
    if (current_ui)
    {
        pthread_mutex_lock(&current_ui->lock);
        detach_locked(current_ui);
        pthread_mutex_unlock(&current_ui->lock);
    }
}
