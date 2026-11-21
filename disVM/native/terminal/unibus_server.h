/* Copyright (c) 2026 bhgv
 * EN: Own a reverse Styx session exposing one Unibus inbox.
 * RU: Встречная сессия Styx с одним приёмником Unibus.
 */
#ifndef STYX_UNIBUS_SERVER_H
#define STYX_UNIBUS_SERVER_H
#include "unibus_inbox.h"
#include "phy.h"

#define STYX_UNIBUS_REPLY_TAG 17UL /* One-shot terminal launch completion tag. */

typedef struct StyxUnibusServer StyxUnibusServer; /* Owned reverse server and receive provider. */


/* EN/RU: Consume channel on every outcome; return owned server or NULL.
 * Принять канал при любом исходе; вернуть свой сервер или NULL. */
StyxUnibusServer *styxunibusserverstart(RemotePhy *channel, unsigned long tag);


/* EN/RU: Also export a borrowed exact original node as read-only implementation.dis.
 * Также опубликовать заимствованный точный узел implementation.dis на чтение. */
StyxUnibusServer *styxunibusserverstartfile(RemotePhy *channel, unsigned long tag, DVM_Provider *original);


/* EN/RU: Also export the optional borrowed profile as read-only .interface.
 * Также опубликовать заимствованный профиль как .interface на чтение. */
StyxUnibusServer *styxunibusserverstartmodule(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);


/* EN/RU: Like startmodule, with a reusable single-record inbox for serialized calls.
 * Как startmodule, с переиспользуемым приёмником для последовательных вызовов. */
StyxUnibusServer *styxunibusserverstartsession(RemotePhy *channel, unsigned long tag,
    DVM_Provider *original, DVM_Provider *profile);


/* EN/RU: Consume channel and publish mounted/module/name, borrowing method state through join.
 * Принять канал и опубликовать mounted/module/name, заимствуя состояние метода до join. */
struct StyxScalarFunction;
StyxUnibusServer *styxunibusserverstartfunction(RemotePhy *channel, const char *module,
    const char *name, const struct StyxScalarFunction *function);


#define STYX_MOUNTED_SCALAR_LIMIT 64U /* Maximum export entries per server. */

typedef struct StyxMountedScalar
{
    const char *module; /* Borrowed directory name. */
    const char *name; /* Borrowed export name. */
    const struct StyxScalarFunction *native; /* Optional copied native method; NULL selects Dis. */
    int dis_big; /* Select the full-width signature for a Dis-backed entry. */
} StyxMountedScalar;


/* EN/RU: Consume channel and publish count borrowed entries grouped by module name.
 * Принять канал и опубликовать count заимствованных записей с группировкой по имени модуля. */
StyxUnibusServer *styxunibusserverstartmounted(RemotePhy *channel,
    const StyxMountedScalar *entries, size_t count);


/* EN/RU: Borrow indexed Dis request (reply=0) or reply mailbox until join.
 * Заимствовать приёмник запроса (reply=0) или ответа Dis по индексу до join. */
DVM_Provider *styxunibusservermailbox(StyxUnibusServer *server, size_t index, int reply);


/* EN/RU: Consume channel and expose a typed function serviced by the owner VM.
 * Принять канал и открыть типизированную функцию, обслуживаемую VM владельца. */
StyxUnibusServer *styxunibusserverstartdis(RemotePhy *channel, const char *module, const char *name);


/* EN/RU: Borrow the write mailbox of a Dis endpoint through server join.
 * Заимствовать приёмник записи конечной точки Dis до завершения сервера. */
DVM_Provider *styxunibusserverreply(StyxUnibusServer *server);


/* EN/RU: Borrow the local reader provider until server join.
 * Заимствовать локальный провайдер чтения до завершения сервера. */
DVM_Provider *styxunibusserverinbox(StyxUnibusServer *server);


/* EN/RU: After peer or carrier shutdown, join and consume server; NULL allowed.
 * После закрытия другой стороны или соединения дождаться и освободить сервер; NULL допустим. */
int styxunibusserverjoin(StyxUnibusServer *server);

#endif
