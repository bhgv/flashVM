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
#define DVM_EXEC_REFUSE UINT32_MAX /* Reply length signalling explicit refusal. */
#define DVM_EXEC_DEADLINE_SECONDS 5 /* Maximum native dispatcher wait for the owner. */

typedef struct ExecOwner ExecOwner; /* One open local control file and its registration. */
typedef struct ExecJob
{
    ExecOwner *owner; /* Borrowed while still registered under the broker lock. */
    void *bytes; /* Borrowed peer request buffer through invoke. */
    size_t size; /* Input bytes and output capacity. */
    size_t used; /* Output prefix on successful completion. */
    uint32_t ticket; /* Exact nonreused identity in this broker. */
    int taken; /* Read once by the owner. */
    int finished; /* Reply, refusal or owner retirement. */
    int accepted; /* Successful reply. */
} ExecJob;

struct ExecOwner
{
    DvmExecBroker *broker; /* Borrowed through the retaining file provider. */
    ExecOwner *next; /* Intrusive registration list, guarded by the broker lock. */
    ExecJob *job; /* Borrowed native waiter through its invocation. */
    uint64_t layer; /* Exact VFS mount layer owned by this registration. */
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


/* EN: Mount an execute-only marker for owner at an absent absolute path.
 * RU: Установить метку только для исполнения на свободном абсолютном пути. */
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


/* EN: Receive or complete one local request via the owner fd.
 * RU: Принять или завершить один локальный запрос через fd владельца. */
FVM_Result dvm_exec_receive(void *state, void *bytes, size_t size, size_t *used);
FVM_Result dvm_exec_answer(void *state, const void *bytes, size_t size);
void dvm_exec_close_owner(void *state);

#endif
