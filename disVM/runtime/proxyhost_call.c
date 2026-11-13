/* Copyright (c) 2026 bhgv
 * [GC:] EN: Admit and return actual rooted arguments to the permanent guest owner.
 * RU: Admit и передать actual rooted arguments постоянному guest owner.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "proxyhost.h"
#include "proxyhost_decl.h"
#include "sys_capture.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>


/* [GC:] EN/RU: Exact NC64 tuple; every pointer slot is caller-mapped.
 * Точный NC64 tuple; каждый pointer slot размечен у caller. */
typedef struct
{
    WORD status;           /* Admission status, not a completion result. */
    Proxyhost_Cell *cell;  /* Extra strong owner of actual target. */
    Proxyhost_Cell *left;  /* Extra strong owner of first ref, or H. */
    Proxyhost_Cell *right; /* Extra strong owner of second ref, or H. */
} BeginResult;


/* [GC:] EN: Clear validated old result owners before admission. Packed output
 * remains mapped; destroy runs outside all native locks.
 * RU: Очистить проверенные result owners до admission. Output остаётся mapped;
 * destroy выполняется вне native locks. */
static void clear_result(void *output)
{
    BeginResult empty = {DVM_REMOTE_INVALID, H, H, H};
    void *owners[3];
    owners[0] = dvm_sys_owner((uchar *)output + offsetof(BeginResult, cell));
    owners[1] = dvm_sys_owner((uchar *)output + offsetof(BeginResult, left));
    owners[2] = dvm_sys_owner((uchar *)output + offsetof(BeginResult, right));
    dvm_sys_owners(owners, 3);
    memmove(output, &empty, sizeof(empty));
    destroy(owners[0]);
    destroy(owners[1]);
    destroy(owners[2]);
}


/* EN/RU: Check local slot/generation before scalar conversion; no heap access.
 * Проверить local slot/generation до scalar conversion, без heap access. */
static int handle(LONG session, WORD slot, LONG generation, DvmRemoteHandle *output)
{
    if ((uint64_t)session != dvm_proxyhost_state.identity || slot < 0 || generation <= 0)
    {
        return 0;
    }
    output->session = dvm_proxyhost_state.identity;
    output->slot = slot;
    output->generation = generation;
    return 1;
}


/* [GC:] EN: Retain every tuple occurrence, including aliases, while exports pin
 * values. Roll back added refs on refusal; no allocation or root gap.
 * RU: Удержать tuple occurrences с aliases при pinned exports. При отказе
 * откатить добавленные refs; без allocation/root gap. */
static int retain_result(BeginResult *result)
{
    void *values[3] = {result->cell, result->left, result->right};
    size_t index;
    for (index = 0; index < 3; index++)
    {
        if (values[index] != H && dvm_refcount_retain_u(&D2H(values[index])->ref) != DVM_REFCOUNT_LIVE)
        {
            while (index)
            {
                index--;
                if (values[index] != H && dvm_refcount_release_u(&D2H(values[index])->ref) != DVM_REFCOUNT_LIVE)
                {
                    abort();
                }
            }
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Admit one callback and return owned actual Cell refs. Guest executes
 * on this same owner, then finishes. Failed staging executes no effect.
 * RU: Admit callback и вернуть owned actual Cell refs. Guest исполняется на
 * том же owner, затем finish. Failed staging не исполняет effect. */
void Proxyhost_begin(void *frame)
{
    F_Proxyhost_begin call;
    DvmRemoteRequest request;
    DvmRemoteManagedCall values;
    BeginResult result = {DVM_REMOTE_INVALID, H, H, H};
    int done;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Proxyhost_begin, ret), sizeof(*call.ret));
    if (sizeof(*call.ret) != sizeof(result))
    {
        error(exBounds);
    }
    clear_result(call.ret);
    memset(&request, 0, sizeof(request));
    if (!dvm_proxyhost_state.adapter || !dvm_proxyhost_state.identity || dvm_proxyhost_state.active || call.id <= 0 ||
        call.operation < Proxyhost_Read || call.operation > Proxyhost_Invoke || call.refs < 0 || call.refs > 2 ||
        !handle(call.session, call.slot, call.generation, &request.target) ||
        (call.refs >= 1 && !handle(call.leftsession, call.left, call.leftgen, &request.refs[0])) ||
        (call.refs == 2 && !handle(call.rightsession, call.right, call.rightgen, &request.refs[1])))
    {
        return;
    }
    request.id = call.id;
    request.operation = call.operation;
    request.kind = call.operation == Proxyhost_Invoke ? DVM_REMOTE_FUNCTION : DVM_REMOTE_VARIABLE;
    request.ref_count = call.refs;
    request.payload_size = sizeof(call.value);
    memmove(request.payload, &call.value, sizeof(call.value));
    result.status = dvm_remote_managed_admit(dvm_proxyhost_state.adapter, &request, &done);
    if (result.status == DVM_REMOTE_OK)
    {
        result.status = dvm_remote_managed_resolve(dvm_proxyhost_state.adapter, request.id, &values);
        if (result.status == DVM_REMOTE_OK)
        {
            result.cell = values.target;
            result.left = call.refs >= 1 ? values.refs[0] : H;
            result.right = call.refs == 2 ? values.refs[1] : H;
            if (!retain_result(&result))
            {
                result.status = DVM_REMOTE_CAPACITY;
            }
        }
        if (result.status == DVM_REMOTE_OK)
        {
            dvm_proxyhost_state.active = request.id;
            dvm_proxyhost_active(1);
        }
        else
        {
            if (dvm_remote_managed_finish(dvm_proxyhost_state.adapter, request.id) != DVM_REMOTE_OK)
            {
                abort();
            }
            result.cell = H;
            result.left = H;
            result.right = H;
        }
    }
    memmove(call.ret, &result, sizeof(result));
}
