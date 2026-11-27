/* Copyright (c) 2026 bhgv
 * EN: Per-movie ownership of the named exchange roots in its local Styx server.
 * RU: Владение именованными корнями обмена фильма в его локальном сервере Styx.
 */
#ifndef FLASHVM_STYXRPC_SESSION_PUBLISH_MESSAGES_H
#define FLASHVM_STYXRPC_SESSION_PUBLISH_MESSAGES_H
#include "flashvm/value.h"
#include "styxrpc/serve.h"

typedef struct ScriptMessageRoot ScriptMessageRoot; /* One published registration. */

typedef struct FVM_StyxRpcPublishedMessages
{
    StyxRpcServer *server;     /* Borrowed on the movie owner until clear. */
    ScriptMessageRoot *roots; /* Roots owned by their pinned script handles. */
    int receiving;            /* Guard mutation during a message callback. */
} FVM_StyxRpcPublishedMessages;


/* [GC:] EN: Expose publishMessages on vfs, borrowing publication state and server.
 * RU: Установить publishMessages на vfs с заимствованием состояния и сервера. */
FVM_Result fvm_styxrpc_publish_messages_install(FVM_Context *context,
    FVM_StyxRpcPublishedMessages *published, StyxRpcServer *server);


/* [GC:] EN: Revoke every registration before server or movie destruction.
 * RU: Отозвать все регистрации до уничтожения сервера или фильма. */
void fvm_styxrpc_publish_messages_clear(FVM_StyxRpcPublishedMessages *published);

#endif
