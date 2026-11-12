/* Copyright (c) 2026 bhgv
 * [GC:] EN: World storage, owner publication and participant lifetime.
 * RU: Состояние мира, публикация владельцев и жизнь участников.
 */
#include "disvm/native_world.h"
#include "channel_sync.h"
#include "world_request.h"
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>


/* [GC:] Registered roots stay alive through detach. Only the owner mutates active
 * register/process contents; collector reads published pointers after parking. */
struct DVM_WorldParticipant
{
    DVM_NativeWorld *world;        /* Borrowed coordination domain. */
    DVM_NativeContext *context;    /* Bound owner; kept alive until detach. */
    DVM_WorldParticipant *next;    /* Membership under the world mutex. */
    void *registers;               /* Stable address inside extended up. */
    size_t register_size;          /* Native byte extent for the visitor. */
    void *process;                 /* Root value published at leave/poll. */
    size_t fast_polls;             /* Owner-only count, never read by the collector. */
    size_t locked_polls;           /* Owner-only slow-path count. */
    unsigned long admission_epoch; /* Epoch of the current interpreter/JIT heap interval. */
    unsigned long read_epoch;      /* Oldest live explicit borrow, or zero. */
    unsigned long read_depth;      /* Nested borrows held by this owner. */
    int active;                    /* Owner may execute mutator instructions. */
    int parked;                    /* Owner is currently waiting for resume. */
    int service;                   /* Collector participant restricted to shared try-entry. */
    int shared;                    /* Active interval is restricted to audited shared work. */
};


/* [GC:] Mutex protects membership, root publication and stop predicates only.
 * It is never held while executing a normal mutator instruction. */
struct DVM_NativeWorld
{
    FVM_PortSync *sync;                 /* Guards state, never instruction bodies. */
    DVM_WorldParticipant *participants; /* Registered roots, including inactive ones. */
    DVM_NativeContext *collector;       /* Bound collecting owner, or null. */
    unsigned long reclaim_epoch;        /* Latest issued reader generation. */
    size_t count;                       /* Number of root sets. */
    size_t active;                      /* Mutators between enter and leave. */
    size_t parked;                      /* Active mutators stopped at poll. */
    size_t shared_peak;                 /* Highest simultaneous restricted mutator count. */
    size_t shared;                      /* Active restricted shared intervals. */
    long request;                       /* Atomic stop request, not root publication. */
    int forbid_stops;                   /* Immutable concurrent domain rejects every collector stop. */
    int sealed;                         /* Registration closed by the last active owner before shutdown. */
    int serial;                         /* Immutable hosted GC admission mode. */
    size_t admissions;                  /* Successful serialized entries under sync. */
    int locked_poll;                    /* Immutable reference-mode selection. */
    int stopped;                        /* Collector may inspect roots. */
    int resuming;                       /* Drain the previous parked generation before another collector. */
};


/* [GC:] EN: Reject cross-thread calls before inspecting owner-only mutable root state.
 * RU: participant заимствован; вернуть признак привязки к текущему потоку без чтения чужого
 * изменяемого состояния. */
static int owns(DVM_WorldParticipant *participant)
{
    return participant && dvm_native_context_current() == participant->context;
}


/* [GC:] EN: Publish the current process at the same boundary as its stable registers.
 * RU: Сохранить текущий process участника participant; вызывать только его владельцем на границе
 * публикации, владение не передаётся. */
static void publish(DVM_WorldParticipant *participant)
{
    participant->process = dvm_native_context_process(participant->context);
}


/* [GC:] EN: An active mutator parks with a complete instruction's roots visible.
 * RU: user — заимствованный участник своего потока. В эталонном режиме ждать возобновления с
 * опубликованными корнями; вернуть ноль при неверном владельце. */
static int poll_mutator(void *user)
{
    DVM_WorldParticipant *participant = (DVM_WorldParticipant *)user;
    DVM_NativeWorld *world;
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    /* Only this owner changes active; the collector reads it under sync.
     * A no-request observation does not publish roots or admit collection. */
    if (!participant->active)
    {
        return 0;
    }
    if (!world->locked_poll && !dvm_world_request_load(&world->request))
    {
        participant->fast_polls++;
        return 1;
    }
    participant->locked_polls++;
    fvm_port_sync_lock(world->sync);
    if (!participant->active)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    publish(participant);
    if (world->collector)
    {
        participant->parked = 1;
        world->parked++;
        fvm_port_sync_signal_all(world->sync);
        while (world->collector)
        {
            fvm_port_sync_wait(world->sync);
        }
        world->parked--;
        participant->parked = 0;
        fvm_port_sync_signal_all(world->sync);
    }
    fvm_port_sync_unlock(world->sync);
    return 1;
}


/* [GC:] EN: Keep the collecting up bound until resume; mutator execution is forbidden here.
 * RU: user заимствован и не используется; всегда вернуть ноль, запрещая исполнение mutator-кода
 * контекстом сборщика. */
static int poll_collector(void *user)
{
    (void)user;
    return 0;
}


/* [GC:] EN: Allocate an owned empty coordination domain, never a VM heap. locked_poll
 * selects the mutex reference route when nonzero. Null reports native allocation
 * failure; no participant or collector may outlive the eventual destruction.
 * RU: locked_poll задаёт проверку под mutex; вернуть собственный пустой домен либо NULL при нехватке
 * памяти. Публиковать только после создания. */
DVM_NativeWorld *dvm_world_create_mode(int locked_poll)
{
    DVM_NativeWorld *world = (DVM_NativeWorld *)calloc(1, sizeof(*world));
    if (!world)
    {
        return 0;
    }
    world->sync = fvm_port_sync_create();
    if (!world->sync)
    {
        free(world);
        return 0;
    }
    world->locked_poll = locked_poll != 0;
    return world;
}


/* [GC:] EN: Create a hosted GC admission domain. locked_poll selects the reference
 * instruction poll. Returns an owned empty world or NULL.
 * RU: locked_poll задаёт polling; вернуть собственный hosted-домен либо NULL. Hosted-протокол не
 * сериализует исполнение разных гостей. */
DVM_NativeWorld *dvm_world_create_serial(int locked_poll)
{
    DVM_NativeWorld *world = dvm_world_create_mode(locked_poll);
    if (world)
    {
        world->serial = 1;
    }
    return world;
}


/* [GC:] EN: Create an owned hosted world with locked_poll selection and no
 * collector exclusion. Any stop attempt terminates with diagnostic exit 70.
 * Return NULL on allocation failure; configure before participant publication.
 * RU: Создать hosted-мир с выбором locked_poll без эксклюзива сборщика.
 * Любая попытка остановки завершает процесс с диагностикой и кодом 70.
 * NULL при нехватке памяти; настройка предшествует публикации участников. */
DVM_NativeWorld *dvm_world_create_concurrent(int locked_poll)
{
    DVM_NativeWorld *world = dvm_world_create_serial(locked_poll);
    if (world)
    {
        world->forbid_stops = 1;
    }
    return world;
}


/* [GC:] EN: Observe immutable stop prohibition on borrowed live world.
 * Return zero for NULL. No admission, state mutation or blocking is involved.
 * RU: Проверить неизменный запрет остановок заимствованного живого world.
 * Для NULL вернуть ноль. Допуска, изменения состояния и ожидания нет. */
int dvm_world_stops_forbidden(DVM_NativeWorld *world)
{
    return world && world->forbid_stops;
}


/* [GC:] EN: Create the default atomic-request domain; null reports native allocation failure.
 * RU: Без аргументов создать собственный домен с атомарным polling; NULL означает нехватку памяти. */
DVM_NativeWorld *dvm_world_create(void)
{
    return dvm_world_create_mode(0);
}


/* [GC:] EN: Caller quiesces outside users before destruction; registered roots forbid it.
 * RU: world передаётся для освобождения только после ухода участников и завершения сборщика. Вернуть
 * ноль без освобождения занятого домена. */
int dvm_world_destroy(DVM_NativeWorld *world)
{
    if (!world)
    {
        return 1;
    }
    fvm_port_sync_lock(world->sync);
    if (world->count || world->collector || world->resuming)
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    fvm_port_sync_unlock(world->sync);
    fvm_port_sync_destroy(world->sync);
    free(world);
    return 1;
}


/* [GC:] EN: Register context roots in world and install their poll hook. Service roots
 * are for a collector that only tries shared admission. Return NULL on refusal.
 * RU: Заимствовать world и текущий context; service выбирает участника shared-службы. Вернуть
 * регистрацию либо NULL при конфликте/нехватке памяти; участник снимается своим владельцем. */
static DVM_WorldParticipant *join_world(DVM_NativeWorld *world, DVM_NativeContext *context, int service)
{
    DVM_WorldParticipant *participant;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!world || !context || dvm_native_context_current() != context)
    {
        return 0;
    }
    participant = (DVM_WorldParticipant *)calloc(1, sizeof(*participant));
    if (!participant)
    {
        return 0;
    }
    participant->service = service;
    participant->world = world;
    participant->context = context;
    participant->registers = dvm_native_context_registers(context);
    participant->register_size = dvm_native_context_register_size(context);
    publish(participant);
    fvm_port_sync_lock(world->sync);
    if (world->sealed || world->collector || !dvm_native_context_set_poll(context, poll_mutator, participant))
    {
        fvm_port_sync_unlock(world->sync);
        free(participant);
        return 0;
    }
    participant->next = world->participants;
    world->participants = participant;
    world->count++;
    fvm_port_sync_unlock(world->sync);
    return participant;
}


/* [GC:] EN: Register ordinary owner context roots in world. Return NULL on refusal.
 * RU: Зарегистрировать текущий context в заимствованном world; вернуть участника либо NULL при
 * отказе. Контекст живёт до detach. */
DVM_WorldParticipant *dvm_world_join(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    return join_world(world, context, 0);
}


/* [GC:] EN: Register a collector context for nonblocking shared work in world.
 * The caller detaches on its own context before the world is destroyed.
 * RU: Зарегистрировать текущий context как shared-службу world без права обычного исполнения.
 * Вернуть участника либо NULL; контекст живёт до detach. */
DVM_WorldParticipant *dvm_world_join_service(DVM_NativeWorld *world, DVM_NativeContext *context)
{
    return join_world(world, context, 1);
}


/* [GC:] EN: Remove only inactive roots on their owner; a collector never sees freed storage.
 * RU: Снять неактивного participant только его потоком; освободить регистрацию при успехе. Ноль при
 * конфликте, активном состоянии или живом borrow. */
int dvm_world_detach(DVM_WorldParticipant *participant)
{
    DVM_NativeWorld *world;
    DVM_WorldParticipant **link;
    if (dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    if (!owns(participant))
    {
        return 0;
    }
    world = participant->world;
    fvm_port_sync_lock(world->sync);
    if (participant->active || world->collector || participant->read_depth ||
        !dvm_native_context_clear_poll(participant->context, poll_mutator, participant))
    {
        fvm_port_sync_unlock(world->sync);
        return 0;
    }
    for (link = &world->participants; *link != participant; link = &(*link)->next)
        ;
    *link = participant->next;
    world->count--;
    fvm_port_sync_unlock(world->sync);
    free(participant);
    return 1;
}
