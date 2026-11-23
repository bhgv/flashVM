/* Copyright (c) 2026 bhgv
 * EN: Lend a bounded graph of existing FlashVM queues under one native reverse PID.
 * RU: Передача ограниченного графа существующих очередей FlashVM под одним встречным PID.
 */
#include "channel_private.h"
#include "flashvm/execution.h"
#include "styxrpc/channel_group.h"
#include <stdlib.h>
#include <string.h>


/* EN: Keep one native arena alive without borrowing its script object.
 * RU: Удерживать отдельную нативную арену независимо от объекта AS. */
typedef struct LocalPublication
{
    StyxRpcChannelQueue *local; /* Original buffer and cursor of the pinned queue. */
    void *owner;                /* Native arena reference released after all fids. */
    size_t sizes[FVM_STYXRPC_CHANNEL_QUEUE_SLOTS]; /* Lengths in the nested staging arena. */
    int nested;                /* A buffered queue with channel leaves needs staging. */
} LocalPublication;


/* EN: Own the registered resource and all of its native queues through revocation.
 * RU: Владеть зарегистрированным ресурсом и всеми нативными очередями до отзыва. */
typedef struct FVM_StyxLocalLoan
{
    struct FVM_StyxLocalLoan *next; /* Next publication on the same result family. */
    StyxRpcServer *reverse;        /* Borrowed reverse dispatcher through retirement. */
    StyxRpcChannelOutput output;   /* Resource with exclusive access to queue storage. */
    StyxRpcChannelQueue queues[STYXRPC_CHANNEL_SLOTS]; /* Descriptors lent to the SDK. */
    LocalPublication members[STYXRPC_CHANNEL_SLOTS]; /* Related original queues and pins. */
    size_t count;                  /* Number of fully reserved member slots. */
    uint32_t pid;                  /* Published PID, or zero before registration. */
} FVM_StyxLocalLoan;


/* [GC:] EN: Restore one pinned queue's native bytes and cursors after dispatcher
 * retirement. The next owning script access resolves the restored channel leaves.
 * RU: Вернуть байты и курсоры удерживаемой очереди после отзыва диспетчером.
 * Владелец разрешит восстановленные ссылки при следующем обращении из AS. */
static void return_member(const LocalPublication *member, const StyxRpcChannelQueue *queue, int issued)
{
    StyxRpcChannelQueue *local = member->local;
    size_t i;
    if (issued)
    {
        local->head = queue->head;
        local->count = queue->count;
        for (i = 0; member->nested && i < queue->count; ++i)
        {
            size_t slot = (queue->head + i) % queue->capacity;
            memcpy(local->bytes + slot * local->stride, queue->bytes + slot * queue->stride, queue->sizes[slot]);
            local->sizes[slot] = queue->sizes[slot];
        }
    }
    if (member->nested)
    {
        free(queue->bytes);
    }
    fvm_styxrpc_local_channel_return(member->owner, issued && member->nested);
}


/* [GC:] EN: Retire loan or roll back partial staging. Use only native memory.
 * RU: Отозвать loan или отменить незавершённую подготовку. Работать только с нативной памятью. */
static void retire_loan(FVM_StyxLocalLoan *loan)
{
    size_t i;
    if (loan->pid)
    {
        styxrpc_program_server_revoke(loan->reverse, loan->pid);
    }
    if (loan->output.release)
    {
        loan->output.release(loan->output.resource.context);
    }
    for (i = 0; i < loan->count; ++i)
    {
        return_member(loan->members + i, loan->queues + i, loan->pid != 0);
    }
    free(loan);
}


/* [GC:] EN/RU: See channel_private.h. / См. channel_private.h. */
void fvm_styxrpc_channel_retire_loans(FVM_StyxChannelSet *set)
{
    while (set->loans)
    {
        FVM_StyxLocalLoan *loan = set->loans;
        set->loans = loan->next;
        retire_loan(loan);
    }
}


/* [GC:] EN: Root value, reserve its key, and exclude local use. Coalesce aliases
 * using objects, which contains only temporary VM roots. Return its queue or NULL.
 * RU: Удержать value, выделить ключ и запретить локальный доступ. Объединить алиасы
 * по objects, где хранятся временные корни VM. Вернуть очередь или NULL. */
static StyxRpcChannelQueue *reserve(FVM_Context *context, FVM_GCRootScope *roots,
    FVM_StyxLocalLoan *loan, FVM_Value *objects, const FVM_Value *value)
{
    LocalPublication *member;
    StyxRpcChannelQueue *queue;
    size_t index;
    if (!value || value->type != FVM_VALUE_OBJECT)
    {
        return NULL;
    }
    for (index = 0; index < loan->count; ++index)
    {
        if (objects[index].object == value->object)
        {
            return loan->queues + index;
        }
    }
    if (index == STYXRPC_CHANNEL_SLOTS || fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return NULL;
    }
    member = loan->members + index;
    member->local = fvm_styxrpc_local_channel_borrow(context, value, &member->owner);
    if (!member->local)
    {
        return NULL;
    }
    objects[index] = *value;
    queue = loan->queues + index;
    *queue = *member->local;
    queue->grant.key = (uint32_t)(index + 1);
    ++loan->count;
    member->nested = queue->capacity && strchr(queue->grant.shape, 'c');
    if (member->nested)
    {
        queue->sizes = member->sizes;
        queue->bytes = malloc(queue->capacity * queue->stride);
        if (!queue->bytes)
        {
            return NULL;
        }
    }
    return queue;
}


/* [GC:] EN: Rebind one occupied slot's exact local edges to the PID's new keys.
 * Newly discovered objects are appended to the loan and its temporary roots.
 * RU: Привязать точные ссылки занятого слота к новым ключам PID.
 * Новые объекты добавляются в loan и временные корни. */
static int rebind_slot(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxLocalLoan *loan,
    FVM_Value *objects, size_t index, size_t slot)
{
    FVM_StyxLocalRefs refs = {0};
    StyxRpcChannelKeyMap keys[STYXRPC_CHANNEL_SLOTS];
    StyxRpcChannelQueue *queue = loan->queues + index;
    const StyxRpcChannelQueue *original = loan->members[index].local;
    size_t offset = slot * queue->stride;
    size_t i;
    fvm_styxrpc_local_slot(context, objects[index].object, slot, &refs, 0);
    for (i = 0; i < STYXRPC_CHANNEL_SLOTS && refs.objects[i]; ++i)
    {
        FVM_Value value;
        const StyxRpcChannelQueue *child;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = refs.objects[i];
        child = reserve(context, roots, loan, objects, &value);
        if (!child)
        {
            return 0;
        }
        keys[i].source = refs.keys[i];
        keys[i].target = child->grant.key;
    }
    return styxrpc_channel_element_remap(queue->grant.shape, original->bytes + offset, original->sizes[slot],
        keys, i, queue->bytes + offset, queue->stride, queue->sizes + slot) == STYXRPC_OK;
}


/* [GC:] EN: Stage all reachable occupied slots and build the shared traced graph
 * before the dispatcher can see the resource. Leave rollback to the caller.
 * RU: Подготовить занятые достижимые слоты и общий трассируемый граф до выдачи ресурса.
 * Отмену при ошибке выполняет вызывающий. */
static FVM_Result prepare(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxLocalLoan *loan,
    FVM_Value *objects, const FVM_Value *value, FVM_Value *graph)
{
    size_t i, j;
    int nested = 0;
    if (!reserve(context, roots, loan, objects, value))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < loan->count; ++i)
    {
        const StyxRpcChannelQueue *queue = loan->queues + i;
        if (!loan->members[i].nested)
        {
            continue;
        }
        nested = 1;
        for (j = 0; j < queue->count; ++j)
        {
            if (!rebind_slot(context, roots, loan, objects, i, (queue->head + j) % queue->capacity))
            {
                return FVM_INVALID_ARGUMENT;
            }
        }
    }
    if (styxrpc_channel_group_reverse_borrow_graph(loan->queues, loan->count, 0, &loan->output) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (nested)
    {
        FVM_Result status = fvm_exec_literal_array(context, (unsigned)loan->count, objects, graph);
        return status == FVM_OK ? fvm_gc_roots_add(roots, graph) : status;
    }
    fvm_value_undefined(graph);
    return FVM_OK;
}


/* [GC:] EN: Publish only after allocating the opaque offer. Commit the graph edges
 * and fill its route without further managed allocation.
 * RU: Опубликовать ресурс после выделения непрозрачного значения. Установить
 * ссылки графа и маршрут без дальнейшего выделения управляемых объектов. */
static FVM_Result publish(FVM_Context *context, FVM_StyxChannelSet *set, FVM_StyxLocalLoan *loan,
    const FVM_Value *objects, const FVM_Value *graph, FVM_Value *out)
{
    FVM_StyxPidGrant *grant;
    FVM_Value offer;
    FVM_Result status = fvm_styxrpc_channel_create_pid(context, set, &grant, &offer);
    size_t index;
    if (status != FVM_OK)
    {
        return status;
    }
    loan->pid = styxrpc_program_server_grant(loan->reverse, &loan->output.resource);
    if (!loan->pid)
    {
        grant->set = NULL;
        fvm_styxrpc_channel_set_release(set, 0);
        return FVM_ERROR;
    }
    for (index = 0; index < loan->count; ++index)
    {
        if (loan->members[index].nested)
        {
            fvm_styxrpc_local_edge(objects[index].object, loan->queues[index].capacity, 0, graph->object, 1);
        }
    }
    grant->route.kind = STYXRPC_ROUTE_PID;
    grant->route.pid = loan->pid;
    grant->route.channel = loan->queues[0].grant;
    grant->route.unbuffered = loan->output.unbuffered;
    loan->next = set->loans;
    set->loans = loan;
    *out = offer;
    return FVM_OK;
}


/* [GC:] EN/RU: See flashvm/styxrpc_channel.h. / См. flashvm/styxrpc_channel.h. */
FVM_Result fvm_styxrpc_channel_delegate_local(FVM_Context *context, const FVM_Value *anchor,
    const FVM_Value *value, StyxRpcServer *reverse, FVM_Value *out)
{
    FVM_StyxLocalLoan *loan;
    FVM_GCRootScope *roots;
    FVM_Value objects[STYXRPC_CHANNEL_SLOTS], graph;
    FVM_Result status;
    if (!reverse || !out || !value || value->type != FVM_VALUE_OBJECT || !fvm_styxrpc_channel_lease_set(context, anchor))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    loan = calloc(1, sizeof(*loan));
    if (loan)
    {
        loan->reverse = reverse;
        status = fvm_gc_roots_add(roots, anchor);
        if (status == FVM_OK)
        {
            status = prepare(context, roots, loan, objects, value, &graph);
        }
        if (status == FVM_OK)
        {
            FVM_StyxChannelSet *set = fvm_styxrpc_channel_lease_set(context, anchor);
            status = set ? publish(context, set, loan, objects, &graph, out) : FVM_INVALID_ARGUMENT;
        }
        if (status != FVM_OK)
        {
            retire_loan(loan);
        }
    }
    else
    {
        status = FVM_OUT_OF_MEMORY;
    }
    fvm_gc_roots_end(roots);
    return status;
}
