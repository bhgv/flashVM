/* Copyright (c) 2026 bhgv
 * EN: Native mailbox ownership shared by Limbo owner fids and the peer dispatcher.
 * RU: Нативная очередь, общая для fid владельца Limbo и диспетчера другой стороны.
 */
#ifndef DVM_EXEC_PRIVATE_H
#define DVM_EXEC_PRIVATE_H
#include "executable_nodes.h"
#include "styxrpc/node_service.h"
#include <pthread.h>

#define DVM_EXEC_WORD 4U /* Local owner protocol field width, bytes. */
#define DVM_EXEC_HEADER (2U * DVM_EXEC_WORD) /* Ticket plus byte length. */
#define DVM_NODE_HEADER (3U * DVM_EXEC_WORD) /* Ticket, byte length and operation. */
#define DVM_NODE_AT_HEADER (DVM_NODE_HEADER + 2U * DVM_EXEC_WORD) /* Adds the UInt64 offset. */
#define DVM_NODE_AT_MODE 0x80U /* Positioned registration adds a UInt64 byte length. */
#define DVM_EXEC_REFUSE UINT32_MAX /* Reply length signalling explicit refusal. */
#define DVM_EXEC_DEADLINE_SECONDS 5 /* Maximum native dispatcher wait for the owner. */

typedef struct ExecOwner ExecOwner; /* One open local control file and its registration. */

/* EN: Mailbox operation carried to the owning Limbo thread.
 * RU: Операция, передаваемая в очередь потока-владельца Limbo. */
typedef enum NodeOperation
{
    DVM_NODE_READ = 'r',  /* The owner fills a zero-initialized buffer. */
    DVM_NODE_WRITE = 'w', /* The owner acknowledges an input prefix. */
    DVM_NODE_EXEC = 'x'   /* The owner processes input and fills a reply. */
} NodeOperation;

/* EN: Distinguish refusal from an unconfirmed operation already taken by the owner.
 * RU: Отличить отказ от операции, которую владелец принял, но не подтвердил. */
typedef enum NodeOutcome
{
    DVM_NODE_REFUSED,     /* No dispatch or an explicit refusal from the owner. */
    DVM_NODE_ACCEPTED,    /* Owner confirmed the prefix length. */
    DVM_NODE_UNCONFIRMED  /* Owner took the request, then disconnected or did not answer in time. */
} NodeOutcome;

/* EN: One waiting native operation, guarded by the broker mutex.
 * RU: Одна ожидающая нативная операция под мьютексом брокера. */
typedef struct ExecJob
{
    ExecOwner *owner; /* Borrowed while still registered under the broker lock. */
    const void *input; /* Borrowed write/execute input, null for reads. */
    void *output; /* Borrowed read/execute output, null for writes. */
    uint64_t offset; /* Absolute data offset, zero for execute. */
    NodeOperation operation; /* Requested callback operation. */
    size_t size; /* Input bytes and output capacity. */
    size_t used; /* Output prefix on successful completion. */
    uint32_t ticket; /* Exact nonreused identity in this broker. */
    int taken; /* Read once by the owner. */
    int answered; /* Reply or refusal from the owner. */
    int accepted; /* Successful reply. */
} ExecJob;

struct ExecOwner
{
    DvmExecBroker *broker; /* Borrowed through the retaining file provider. */
    ExecOwner *next; /* Intrusive registration list, guarded by the broker lock. */
    ExecJob *job; /* Borrowed native waiter through its invocation. */
    uint64_t layer; /* Exact VFS mount layer owned by this registration. */
    uint32_t marker; /* Exact session-local marker identity. */
    unsigned mode; /* FVM_ACCESS bits permitted by this publication. */
    uint64_t length; /* Declared fixed byte length of a positioned node. */
    int positioned; /* Exchange the extra 64-bit offset in each request. */
    int extended; /* Use the operation-bearing local mailbox format. */
    char path[STYXRPC_NODE_PATH_MAX + 2]; /* Canonical absolute publication path. */
};

struct DvmExecBroker
{
    DVM_Namespace *group; /* Borrowed while launch pins the namespace. */
    pthread_mutex_t lock; /* Protects native owners, jobs and lifetime. */
    pthread_cond_t changed; /* Signals completion or owner retirement. */
    ExecOwner *owners; /* Non-owning list of active registrations. */
    unsigned references; /* Launcher and retaining file providers. */
    uint32_t ticket; /* Last assigned ticket; refuse after exhaustion. */
    uint32_t marker; /* Last assigned marker identity; never reuse it within the broker. */
    int stopped; /* Set before the caller joins the dispatcher. */
};


/* EN: Mount the owner's node at an absent absolute path with its registered permissions.
 * RU: Установить узел владельца на свободном абсолютном пути с заданными правами. */
int dvm_exec_claim(ExecOwner *owner, const char *absolute);


/* EN: Read the currently selected layer for an absolute path or zero.
 * RU: Прочитать выбранный сейчас слой абсолютного пути либо ноль. */
uint64_t dvm_exec_layer(DvmExecBroker *broker, const char *absolute);


/* EN: Remove the owner's exact marker without removing a replacement.
 * RU: Снять точную метку владельца, сохранив возможную замену. */
void dvm_exec_unclaim(ExecOwner *owner);


/* EN: Invoke one named owner with the shared wire codec's in-place request buffer.
 * RU: Вызвать одного владельца с изменяемым буфером запроса от общего кодека. */
int dvm_exec_invoke(void *context, const char *path, void *bytes, size_t size, size_t *used);


/* EN: Hold the broker independently of the owning fid while a provider is reachable.
 * RU: Удерживать брокер независимо от fid владельца, пока доступен провайдер. */
void dvm_exec_retain(DvmExecBroker *broker);
void dvm_exec_release(void *state);


/* EN: Check that the exact marker still belongs to an active owner.
 * RU: Проверить, что точная метка по-прежнему принадлежит активному владельцу. */
int dvm_exec_active(DvmExecBroker *broker, uint32_t marker);


/* EN: Wait for the exact owner to process one borrowed buffer and acknowledge its prefix.
 * RU: Дождаться обработки одного заимствованного буфера и подтверждения префикса. */
NodeOutcome dvm_exec_transfer(DvmExecBroker *broker, uint32_t marker, NodeOperation operation,
    uint64_t offset, const void *input, void *output, size_t size, size_t *used);


/* EN: Receive or complete one local request via the owner fd.
 * RU: Принять или завершить один локальный запрос через fd владельца. */
FVM_Result dvm_exec_receive(void *state, void *bytes, size_t size, size_t *used);
FVM_Result dvm_exec_answer(void *state, const void *bytes, size_t size);
void dvm_exec_close_owner(void *state);

#endif
