/* Copyright (c) 2026 bhgv
 * EN: Application-defined UniBus messages on an attached Styx session.
 * RU: Пользовательские сообщения UniBus в подключённой сессии Styx.
 */
#ifndef STYXRPC_MESSAGES_H
#define STYXRPC_MESSAGES_H
#include "styxrpc/serve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_MESSAGES_FILE ".messages"  /* Compatibility path for enable/open. */
#define STYXRPC_UNIBUS_FILE ".unibus"      /* Default name for publish/open_at. */
#define STYXRPC_MESSAGE_TYPES_MAX 32U     /* Maximum declarations in either direction. */
#define STYXRPC_MESSAGE_NAME_MAX 63U      /* Maximum identity bytes, excluding NUL. */
#define STYXRPC_MESSAGE_PAYLOAD_MAX 16384U /* Maximum payload bytes in one record. */
#define STYXRPC_MESSAGE_ROOTS_MAX 16U     /* Named exchange roots per hosted server. */
#define STYXRPC_MESSAGES_PORTS_MAX 8U      /* Maximum simultaneous ports per exchange root. */

struct StyxClient; /* An attached Styx session, owned by its caller. */
typedef struct StyxRpcMessagePort StyxRpcMessagePort; /* One duplex stream on one open fid. */


/* EN: Dispatch borrowed payload bytes to the registered handler for this identity/version.
 * Return 1 to accept, or 0 to fail the port. Copy retained bytes. May queue a send on port.
 * RU: Передать заимствованные байты обработчику данного имени и версии.
 * Вернуть 1 при успехе или 0 при ошибке порта. Для хранения скопировать данные.
 * Можно поставить отправку в очередь port. */
typedef int (*StyxRpcMessageHandler)(StyxRpcMessagePort *port, void *context,
    const void *payload, size_t size);


/* EN: Register one schema in a port's local send and/or receive table.
 * RU: Зарегистрировать схему в локальной таблице отправки и/или приёма порта. */
typedef struct StyxRpcMessageType
{
    const char *name;             /* Exact, case-sensitive ASCII 0x21..0x7e, 1..63 bytes. */
    uint32_t version;             /* Nonzero schema version. */
    uint32_t send_tag;            /* Direction-local numeric tag, 0 for receive-only. */
    StyxRpcMessageHandler receive; /* NULL for send-only. */
    void *context;                /* Borrowed receiver state. */
} StyxRpcMessageType;


/* EN: Describe a fixed registry and optional hosted-server lifecycle hooks.
 * RU: Описать постоянный реестр и необязательные серверные хуки жизненного цикла. */
typedef struct StyxRpcMessageSpec
{
    const StyxRpcMessageType *types; /* Copied registrations and names. */
    size_t count;                   /* 1..STYXRPC_MESSAGE_TYPES_MAX entries. */
    void (*opened)(StyxRpcMessagePort *port, void *context); /* After open, may queue a send. */
    void (*closed)(StyxRpcMessagePort *port, void *context); /* Before disposal, port cannot send. */
    void *context;                  /* Borrowed hook state; ignored on the client. */
} StyxRpcMessageSpec;


/* EN: Publish a named exchange root before the first server step. Copy name and spec.
 * Name is one root child component, or NULL for .unibus. Return INVALID for a collision or invalid arguments.
 * RU: Опубликовать именованный корень обмена до первого шага. Скопировать name и spec.
 * Имя — один компонент в корне или NULL для .unibus. Коллизия или неверные аргументы дают INVALID. */
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec);


/* EN: Open a named message root on session (NULL name selects .unibus). Ownership and results match messages_open.
 * RU: Открыть именованный корень в session (NULL выбирает .unibus). Владение и результат как у messages_open. */
StyxRpcStatus styxrpc_messages_open_at(struct StyxClient *session, const char *name,
    const StyxRpcMessageSpec *spec, StyxRpcMessagePort **out);


/* EN: Copy spec and publish the message endpoint before the server's first step.
 * Return OK or INVALID. The callbacks and their contexts must remain valid through
 * destruction. Use the server owner thread for hooks, sends and receives.
 * RU: Скопировать spec и опубликовать узел сообщений до первого шага сервера.
 * Результат OK либо INVALID. Колбэки и их контексты действуют до уничтожения.
 * Хуки, отправка и приём выполняются в потоке владельца сервера. */
StyxRpcStatus styxrpc_messages_enable(StyxRpcServer *server, const StyxRpcMessageSpec *spec);


/* EN: Open a message fid on session, copy spec, and publish an owned port on OK.
 * INVALID leaves out unchanged. Only the owner operates the port. Keep callback
 * contexts alive until close. Client specs use types/count only.
 * RU: Открыть fid сообщений в session, скопировать spec и при OK выдать собственный порт.
 * При INVALID out не меняется. С портом работает владелец. Контексты действуют
 * до закрытия. Клиент использует только types/count. */
StyxRpcStatus styxrpc_messages_open(struct StyxClient *session, const StyxRpcMessageSpec *spec,
    StyxRpcMessagePort **out);


/* EN: Copy payload bytes into the port's bounded send queue by exact name/version.
 * OK means queued locally, MORE means queue full, INVALID means bad input or failed port.
 * Owner only, including from the receive/open callback. No I/O in this call.
 * RU: Скопировать payload в ограниченную очередь по точному имени и версии.
 * OK означает постановку в очередь, MORE — очередь заполнена, INVALID — ошибка
 * аргумента или порта. Только владелец, включая колбэк приёма/открытия. Без I/O. */
StyxRpcStatus styxrpc_message_send(StyxRpcMessagePort *port, const char *name, uint32_t version,
    const void *payload, size_t size);


/* EN: Flush queued bytes and read one bounded batch on an open client port.
 * OK means at least one user message delivered, MORE means none ready. An I/O,
 * protocol or handler error returns INVALID and retires the port. Uses normal Styx
 * request deadlines. Owner only, outside callbacks on the same port/session.
 * RU: Передать очередь и прочитать одну порцию на открытом клиентском порту.
 * OK означает доставку хотя бы одного сообщения, MORE — готовых нет. Ошибка I/O,
 * протокола или обработчика даёт INVALID и выводит порт из работы. Использует
 * сроки ожидания Styx. Только владелец, вне колбэков этого порта/сессии. */
StyxRpcStatus styxrpc_messages_pump(StyxRpcMessagePort *port);


/* EN: Consume a client port and its fid, dropping queued bytes. NULL is harmless.
 * Call on the owner thread outside callbacks on the same port/session.
 * RU: Освободить клиентский порт и fid, отбросив очередь. NULL допустим.
 * Вызывать в потоке владельца вне колбэков этого порта/сессии. */
void styxrpc_messages_close(StyxRpcMessagePort *port);


/* EN: Dispose of an idle client port without transport I/O. The session still
 * owns the deferred clunk and must outlive this call. NULL is harmless.
 * RU: Освободить свободный клиентский порт без транспортного I/O. Сессия
 * владеет отложенным clunk и должна пережить вызов. NULL допустим. */
void styxrpc_messages_abandon(StyxRpcMessagePort *port);

#ifdef __cplusplus
}
#endif
#endif
