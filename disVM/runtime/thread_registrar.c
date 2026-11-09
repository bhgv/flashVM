/* Copyright (c) 2026 bhgv
 * EN: Start and join OS workers through a sleeping native registrar.
 * RU: Запуск и присоединение рабочих потоков через спящий нативный регистратор.
 */
#include "thread_registrar.h"
#include "owner_messages.h"
#include <pthread.h>
#include <stdlib.h>

#define REGISTRAR_ID 1 /* Router key reserved for the registrar. */

enum {
    THREAD_START = 1, /* Start a worker and register its handle. */
    THREAD_JOIN       /* Join a worker and drop its handle. */
};


/* EN: Native handle owned exclusively by the registrar until join.
 * RU: Нативный дескриптор во владении регистратора до join. */
struct DVM_ThreadSlot
{
    DVM_ThreadSlot *next;   /* Registrar-only list link. */
    FVM_PortThread *handle; /* Platform thread handle, closed by join. */
};


/* EN: One registry and its request endpoint; only the registrar edits slots.
 * RU: Реестр и приёмник запросов. Слоты меняет только регистратор. */
struct DVM_ThreadRegistrar
{
    DVM_MessageRouter *router;      /* Routes requests and per-call replies. */
    DVM_MessageEndpoint *requests;  /* Registrar's own inbox. */
    DVM_ThreadSlot *slots;           /* Native handles still owned by the registrar. */
    pthread_t thread;               /* Registrar thread, joined during shutdown. */
};


/* EN: Caller-owned command, live until the reply has arrived.
 * RU: Команда вызывающего потока. Живёт до получения ответа. */
typedef struct DVM_ThreadCommand
{
    DVM_OwnerMessage message; /* Reused by the registrar for its reply. */
    FVM_PortThreadFn fn;      /* Native worker entry for a start request. */
    void *arg;                /* Opaque native argument passed through to fn. */
    DVM_ThreadSlot *slot;     /* Start output or join input. */
} DVM_ThreadCommand;


/* EN: Create a worker for command and retain the returned native handle.
 * RU: Создать поток по command и сохранить его нативный дескриптор. */
static int start_slot(DVM_ThreadRegistrar *registrar, DVM_ThreadCommand *command)
{
    DVM_ThreadSlot *slot = calloc(1, sizeof(*slot));
    if (!slot)
    {
        return 0;
    }
    slot->handle = fvm_port_thread_start(command->fn, command->arg);
    if (!slot->handle)
    {
        free(slot);
        return 0;
    }
    slot->next = registrar->slots;
    registrar->slots = slot;
    command->slot = slot;
    return 1;
}


/* EN: Join a registered slot and remove it from the native registry.
 * RU: Присоединить зарегистрированный slot и снять его с учёта. */
static int join_slot(DVM_ThreadRegistrar *registrar, DVM_ThreadSlot *slot)
{
    DVM_ThreadSlot **entry = &registrar->slots;
    while (*entry && *entry != slot)
    {
        entry = &(*entry)->next;
    }
    if (!*entry)
    {
        return 0;
    }
    fvm_port_thread_join(slot->handle);
    *entry = slot->next;
    free(slot);
    return 1;
}


/* EN: Sleep until a command arrives. No Dis context or heap is accessed here.
 * RU: Спать до команды. Контекст Dis и куча здесь не используются. */
static void *serve(void *argument)
{
    DVM_ThreadRegistrar *registrar = argument;
    DVM_OwnerMessage *message;
    while ((message = dvm_messages_take(registrar->requests, 0, 1)))
    {
        DVM_ThreadCommand *command = (DVM_ThreadCommand *)message;
        uintptr_t sender = message->sender;
        message->value = message->kind == THREAD_START ?
            start_slot(registrar, command) : join_slot(registrar, command->slot);
        if (!dvm_messages_send(registrar->router, sender, 1, message))
        {
            abort();
        }
    }
    return NULL;
}


/* EN: Start the registrar before any caller can send work.
 * RU: Запустить регистратор до публикации запросов. */
DVM_ThreadRegistrar *dvm_thread_registrar_create(void)
{
    DVM_ThreadRegistrar *registrar = calloc(1, sizeof(*registrar));
    if (!registrar)
    {
        return NULL;
    }
    registrar->router = dvm_messages_create();
    if (!registrar->router)
    {
        free(registrar);
        return NULL;
    }
    registrar->requests = dvm_messages_register(registrar->router, REGISTRAR_ID);
    if (!registrar->requests || pthread_create(&registrar->thread, NULL, serve, registrar))
    {
        if (registrar->requests)
        {
            dvm_messages_stop(registrar->requests);
            dvm_messages_unregister(registrar->router, registrar->requests);
        }
        dvm_messages_destroy(registrar->router);
        free(registrar);
        return NULL;
    }
    return registrar;
}


/* EN: Stop the sleeping registrar; all worker handles must be joined first.
 * RU: Остановить спящий регистратор после присоединения всех рабочих потоков. */
void dvm_thread_registrar_destroy(DVM_ThreadRegistrar *registrar)
{
    dvm_messages_stop(registrar->requests);
    if (pthread_join(registrar->thread, NULL) || registrar->slots ||
        !dvm_messages_unregister(registrar->router, registrar->requests))
    {
        abort();
    }
    dvm_messages_destroy(registrar->router);
    free(registrar);
}


/* EN: Send one command and wait on its private, temporary reply FIFO.
 * RU: Отправить команду и дождаться ответа во временном личном FIFO. */
static int roundtrip(DVM_ThreadRegistrar *registrar, DVM_ThreadCommand *command)
{
    uintptr_t id = (uintptr_t)command;
    DVM_MessageEndpoint *reply = dvm_messages_register(registrar->router, id);
    if (!reply)
    {
        return 0;
    }
    command->message.sender = id;
    command->message.request = id;
    int result = 0;
    if (dvm_messages_send(registrar->router, REGISTRAR_ID, 0, &command->message))
    {
        DVM_OwnerMessage *answer = dvm_messages_take(reply, 1, 1);
        if (answer != &command->message)
        {
            abort();
        }
        result = (int)answer->value;
    }
    dvm_messages_stop(reply);
    if (!dvm_messages_unregister(registrar->router, reply))
    {
        abort();
    }
    return result;
}


/* EN: Return a registered native slot on successful worker creation.
 * RU: Вернуть зарегистрированный слот после успешного запуска. */
DVM_ThreadSlot *dvm_thread_registrar_start(DVM_ThreadRegistrar *registrar,
    FVM_PortThreadFn fn, void *arg)
{
    DVM_ThreadCommand command = {0};
    command.message.kind = THREAD_START;
    command.fn = fn;
    command.arg = arg;
    return roundtrip(registrar, &command) ? command.slot : NULL;
}


/* EN: Ask the registrar to join slot; success consumes the handle.
 * RU: Попросить регистратор присоединить slot. Успех освобождает дескриптор. */
int dvm_thread_registrar_join(DVM_ThreadRegistrar *registrar, DVM_ThreadSlot *slot)
{
    DVM_ThreadCommand command = {0};
    command.message.kind = THREAD_JOIN;
    command.slot = slot;
    return roundtrip(registrar, &command);
}
