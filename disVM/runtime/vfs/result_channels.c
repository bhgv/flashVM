/* Copyright (c) 2026 bhgv
 * [GC:] EN: Keep provider-owned returned channels live until the exact forward lease is revoked.
 * RU: Удержание возвращённых каналов провайдера до отзыва точной прямой аренды.
 */
#include "result_channels.h"

#ifdef DVM_TERMINAL_LAUNCH
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runt.h"
#include "channel_proxy.h"
#include "launch/runtime.h"
#include "native_wait.h"
#include "root_registry.h"
#include "runqueue.h"
#include "runtime_gc.h"
#include "thread_port.h"
#include "../../native/terminal/channel_export.h"
#include <stdlib.h>

enum
{
    SERVICE_FRAME = 64, /* An empty native frame with the ordinary saved state slots. */
    SERVICE_REG = 64, /* Offset of the native call frame pointer in the outer frame. */
    SERVICE_SIGNATURE = 1 /* Exact private builtin link signature. */
};


/* [GC:] EN: Rooted local channels and a native bus transferred to one dedicated Prog.
 * RU: Укоренённые локальные каналы и нативная шина для одного выделенного Prog. */
typedef struct ResultChannelService
{
    DVM_CallChannels channels; /* Owned roots, entirely inside this provider VM. */
    DVM_ChannelBus *bus; /* Borrowed from the retained provider below. */
    DVM_Provider *provider; /* Owns the bus independently of the forward lease. */
    DVM_Cancel *cancel; /* Owned forward lease cancellation. */
} ResultChannelService;


/* EN: Borrow current invocation context and return the owned native publication.
 * RU: Заимствовать текущий контекст вызова и вернуть свою нативную публикацию. */
static DVM_Provider *publication(void)
{
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    return owner ? dvm_provider_call_export(owner->remote_call_context) : NULL;
}


/* EN/RU: See result_channels.h. / См. result_channels.h. */
int dvm_result_channels_token(unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE])
{
    DVM_Provider *control = publication();
    int valid = styxchannel_export_token(control, token);
    dvm_provider_release(control);
    return valid;
}


/* [GC:] EN: Consume the service and drop all roots on the retiring owner.
 * RU: Освободить службу и снять корни на завершающемся исполнителе. */
static void service_retire(void *state, int status)
{
    ResultChannelService *service = state;
    (void)status;
    dvm_channel_bus_seal(service->bus);
    dvm_call_channels_release(&service->channels);
    dvm_provider_release(service->provider);
    dvm_cancel_release(service->cancel);
    free(service);
}


/* EN: Run operations on the installed resident until the caller clunks its lease.
 * RU: Выполнять операции на этом исполнителе до закрытия арендующего fid. */
static void service_serve(void *frame)
{
    (void)frame;
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    ResultChannelService *service = owner->report_launch_state;
    DVM_VfsWait *wait;
    int native = dvm_vfs_path_enter_cancel(&wait, service->cancel);
    if (native < 0)
    {
        return;
    }
    while (!dvm_cancel_requested(dvm_cancel_current()) &&
        dvm_call_channel_progress(service->bus, &service->channels, &native))
    {
        fvm_port_thread_sleep_us(1000);
    }
    dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(native);
}


/* [GC:] EN: Keep partially built module ownership across protected startup.
 * RU: Удерживать частично созданные модули через границу защищённого запуска. */
typedef struct ServiceStartup
{
    ResultChannelService *service; /* Borrowed until child publication succeeds. */
    Module *module; /* Owned outer Dis module through child publication. */
    Module *native; /* Owned unlinked native module, or NULL after linking. */
    Modlink *helper; /* Owned helper until placed in the GC-mapped module data. */
    int helper_root; /* Whether the temporary helper pin needs release. */
} ServiceStartup;


/* [GC:] EN: Assemble a private entry calling the native pump once and exiting.
 * RU: Собрать приватную точку входа, один раз вызывающую нативную службу перед выходом. */
static int launch_service(void *state)
{
    ServiceStartup *startup = state;
    Module *native = startup->native = newmod("$StyxReturnedChannelPump");
    native->type = calloc(1, sizeof(*native->type));
    native->ext = calloc(2, sizeof(*native->ext));
    if (!native->type || !native->ext)
    {
        return 0;
    }
    Type *entry = dtype(freeheap, SERVICE_FRAME, NULL, 0);
    if (!entry)
    {
        return 0;
    }
    native->type[native->ntype++] = entry;
    runtime(native, native->ext, "serve", SERVICE_SIGNATURE, service_serve, entry);
    Import imports[] = {{SERVICE_SIGNATURE, "serve"}, {0, NULL}};
    startup->native = NULL;
    startup->helper = linkmod(native, imports, 0);
    if (startup->helper == H || !dvm_runtime_root_add(D2H(startup->helper)))
    {
        return 0;
    }
    startup->helper_root = 1;

    Module *module = startup->module = newmod("$StyxReturnedChannel");
    module->type = calloc(2, sizeof(*module->type));
    module->prog = calloc(3, sizeof(*module->prog));
    if (!module->type || !module->prog)
    {
        return 0;
    }
    Type *frame = dtype(freeheap, SERVICE_REG + sizeof(void *), NULL, 0);
    if (!frame)
    {
        return 0;
    }
    module->type[module->ntype++] = frame;
    unsigned char map = 0x80;
    Type *mp = dtype(freeheap, sizeof(Modlink *), &map, 1);
    if (!mp)
    {
        return 0;
    }
    module->type[module->ntype++] = mp;
    module->mptype = 1;
    Modlink **data = H2D(Modlink **, heapz(mp));
    module->origmp = (uchar *)data;
    Setmark(D2H(startup->helper));
    *data = startup->helper;
    ptrdel(D2H(startup->helper));
    startup->helper_root = 0;
    startup->helper = H;

    module->entryt = frame;
    module->entry = module->prog;
    module->nprog = 3;
    module->prog[0] = (Inst){.op = IMFRAME, .add = SRC(AMP) | AXIMM | DST(AFP), .d.ind = SERVICE_REG};
    module->prog[1] = (Inst){.op = IMCALL, .add = SRC(AFP) | AXIMM | DST(AMP), .s.ind = SERVICE_REG};
    module->prog[2] = (Inst){.op = IEXIT, .add = SRC(AXXX) | DST(AXXX)};
    DVM_LaunchCompletion completion = {.report = service_retire, .state = startup->service};
    return dvm_runtime_launch_child(module, NULL, &completion) != NULL;
}


/* EN/RU: See result_channels.h. / См. result_channels.h. */
int dvm_result_channels_finish(DVM_CallChannels *channels)
{
    DVM_Provider *control = publication();
    ResultChannelService *service = control ? calloc(1, sizeof(*service)) : NULL;
    if (service)
    {
        service->cancel = styxchannel_export_cancel(control);
        service->provider = dvm_channel_bus_create(channels->schemas, channels->count, &service->bus);
    }
    int ready = service && service->cancel && service->provider &&
        styxchannel_export_publish(control, service->provider);
    dvm_provider_release(control);
    if (!ready)
    {
        if (service)
        {
            service_retire(service, 1);
        }
        return 0;
    }
    service->channels = *channels;
    service->channels.resolving = 1;
    service->channels.returning = 0;
    *channels = (DVM_CallChannels){0};
    ServiceStartup startup = {.service = service, .helper = H};
    int started = kprotect(launch_service, &startup);
    if (startup.helper_root)
    {
        ptrdel(D2H(startup.helper));
    }
    destroy(startup.helper);
    if (startup.native)
    {
        unload(startup.native);
    }
    if (startup.module)
    {
        unload(startup.module);
    }
    if (!started)
    {
        dvm_cancel_request(service->cancel);
        service_retire(service, 1);
    }
    return started;
}

#else

/* EN/RU: No mounted service in the standalone runtime. / В автономной VM нет смонтированной службы. */
int dvm_result_channels_token(unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE])
{
    (void)token;
    return 0;
}


/* EN/RU: Preserve staged ownership in the standalone runtime. / Сохранить владение в автономной VM. */
int dvm_result_channels_finish(DVM_CallChannels *channels)
{
    (void)channels;
    return 0;
}
#endif
