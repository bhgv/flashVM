/* Copyright (c) 2026 bhgv
 * [GC:] EN: Current resident's inherited native terminal grant becomes an exact managed capability.
 * RU: Native terminal grant своего resident становится exact managed capability.
 */
#include "cap_internal.h"
#include "runqueue.h"
#include "launch/runtime.h"


/* EN/RU: Borrow immutable native context on its own active owner; no allocation or foreign PID.
 * Заимствовать immutable native context своего active owner; без allocation/foreign PID. */
static DVM_TerminalChannels *channels(void)
{
    Prog *process = currun();
    return process ? ((DVM_RuntimeProcess *)process)->terminal_channels : NULL;
}


/* [GC:] EN/RU: Publish independently retained exact root or nil; current process pins source.
 * Выдать retained exact root/nil; текущий process удерживает source, I/O отсутствует. */
void Vfsexport_terminal(void *frame)
{
    F_Vfsexport_terminal call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_terminal, ret), sizeof(*call.ret));
    DVM_TerminalChannels *source = channels();
    if (!source)
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    DVM_CapState *state;
    void *value = dvm_cap_new(DVM_CAP_ROOT, &state);
    if (!state || !dvm_provider_retain(source->provider))
    {
        dvm_cap_abandon(value);
        value = H;
    }
    else
    {
        state->root = source->provider;
    }
    dvm_cap_publish(value, call.ret);
}


/* [GC:] EN/RU: Copy native channel name as managed string, safely replacing a prior result owner.
 * Скопировать native имя в managed string с guarded заменой предыдущего result owner. */
void Vfsexport_terminalpath(void *frame)
{
    F_Vfsexport_terminalpath call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_terminalpath, ret), sizeof(*call.ret));
    DVM_TerminalChannels *source = channels();
    if (!source || (call.direction != 0 && call.direction != 1))
    {
        dvm_cap_publish(H, call.ret);
        return;
    }
    char *name = call.direction ? source->send : source->receive;
    String *value = c2string(name, strlen(name));
    volatile int rooted = 0;
    if (waserror())
    {
        if (rooted)
        {
            ptrdel(D2H(value));
        }
        destroy(value);
        nexterror();
    }
    ptradd(D2H(value));
    rooted = 1;
    Setmark(D2H(value));
    poperror();
    dvm_cap_publish(value, call.ret);
}
