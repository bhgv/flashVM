/* Copyright (c) 2026 bhgv
 * EN: Named rendezvous channels on an attached Styx session.
 * RU: Именованные каналы без буфера в подключённой сессии Styx.
 */
#ifndef STYXRPC_NAMED_CHANNEL_H
#define STYXRPC_NAMED_CHANNEL_H
#include <stddef.h>
#include <stdint.h>
#include "styxrpc/vfs_client.h"

#define STYXRPC_NAMED_SERVICE ".channels" /* Private service at the attachment root. */
#define STYXRPC_NAMED_MAX STYXRPC_VFS_TRANSFER_MAX /* Maximum counted value, in bytes. */
#define STYXRPC_NAMED_OPEN 1U /* Request body is a canonical absolute VFS path. */
#define STYXRPC_NAMED_READ 2U /* No body. Wait for an opposite write. */
#define STYXRPC_NAMED_WRITE 3U /* Nonempty encoded value. Wait for an opposite read. */
#define STYXRPC_NAMED_HEADER 5U /* Operation followed by big-endian UInt32 size. */
#define STYXRPC_NAMED_REPLY 4U /* Big-endian UInt32 size, zero acknowledges write. */
#define STYXRPC_NAMED_VALUE 1U /* FlashVM graph snapshot. */
#define STYXRPC_NAMED_INT_REF 2U /* Existing StyxRPC Int32 cell descriptor. */
#define STYXRPC_NAMED_NUMBER 3U /* Big-endian binary64. */
#define STYXRPC_NAMED_TEXT 4U /* Counted UTF-8/WTF-8. */
#define STYXRPC_NAMED_BOOLEAN 5U /* One byte, zero or one. */
#define STYXRPC_NAMED_NULL 6U /* Empty null body. */
#define STYXRPC_NAMED_UNDEFINED 7U /* Empty undefined body. */
#define STYXRPC_NAMED_MODULE_REF 9U /* Scoped reference to a live hosted .var export. */
#define STYXRPC_NAMED_NUMBER_BYTES 8U /* Fixed width of one IEEE-754 binary64. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcNamedRegistry StyxRpcNamedRegistry; /* Owns the live name reservations. */
typedef struct StyxRpcNamedEndpoint StyxRpcNamedEndpoint; /* One server-side participant/fid. */
typedef uint64_t (*StyxRpcNamedClaim)(void *state, const char *path); /* Own the new name; zero refuses it. */
typedef int (*StyxRpcNamedVisible)(void *state, const char *path, uint64_t lease); /* Is the claimed name still visible? */
typedef void (*StyxRpcNamedDrop)(void *state, const char *path, uint64_t lease); /* Retire the exact claimed name. */

/* EN: Create a registry. Optional claim/drop hooks run under the registry name lock and
 * must not reenter it. visible can refuse a join after a namespace replacement.
 * Keep state alive until destroy.
 * RU: Создать реестр. Необязательные claim/drop выполняются под блокировкой
 * имён реестра без повторного входа. visible может отклонить вход после замены
 * привязки. state должен жить до destroy. */
StyxRpcNamedRegistry *styxrpc_named_registry(StyxRpcNamedClaim claim,
    StyxRpcNamedVisible visible, StyxRpcNamedDrop drop, void *state);

/* EN: Release the registry after all server endpoints are retired.
 * RU: Освободить реестр после завершения всех серверных дескрипторов. */
void styxrpc_named_destroy(StyxRpcNamedRegistry *registry);

/* EN: Check whether path is a canonical absolute ordinary pathname.
 * RU: Проверить, что path — канонический абсолютный обычный путь. */
int styxrpc_named_path(const char *path);

/* EN: Allocate an unregistered endpoint for one open control fid.
 * RU: Выделить ещё не зарегистрированного участника для одного открытого fid. */
StyxRpcNamedEndpoint *styxrpc_named_endpoint(StyxRpcNamedRegistry *registry);

/* EN: Drop the endpoint and its pending operation. Last endpoint removes its name.
 * RU: Освободить участника и его ожидающую операцию. Последний удаляет имя. */
void styxrpc_named_retire(StyxRpcNamedEndpoint *endpoint);

/* EN: Submit one contiguous fragment, acknowledging it or rejecting the whole operation.
 * RU: Принять последовательный фрагмент либо отклонить всю операцию. */
int styxrpc_named_submit(StyxRpcNamedEndpoint *endpoint, uint64_t offset, const void *bytes, size_t size);

/* EN: Copy a confirmed reply fragment. A zero count means an unresolved rendezvous.
 * RU: Скопировать фрагмент подтверждения. Нулевой размер означает ожидание пары. */
int styxrpc_named_reply(StyxRpcNamedEndpoint *endpoint, uint64_t offset, void *bytes, size_t capacity, size_t *used);

/* EN: Open or create a channel by absolute path; transfer one owned fid on success.
 * RU: Открыть или создать канал по абсолютному пути; при успехе выдать свой fid. */
StyxRpcVfsFile *styxrpc_named_open(struct StyxClient *session, const char *path);

/* EN: Wait on file for one consumed nonempty frame. Own the result via free.
 * RU: Дождаться одной принятой непустой записи. Результат освобождается через free. */
StyxRpcIoOutcome styxrpc_named_recv(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size);

/* EN: Wait until a reader pairs with this write. Serialize operations on each fid.
 * RU: Дождаться пары для этой записи. Сериализовать операции на каждом fid. */
StyxRpcIoOutcome styxrpc_named_send(StyxRpcVfsFile *file, const void *bytes, size_t size);


/* EN: Return nonzero to stop waiting on the calling thread. Do not use file in this hook.
 * RU: Вернуть ненулевое значение для остановки ожидания в вызывающем потоке.
 * Не обращаться к file из этого колбэка. */
typedef int (*StyxRpcNamedStop)(void *state);


/* EN: Receive like styxrpc_named_recv, checking optional stop before submission
 * and between reply polls. On CANCELLED close file without reusing it.
 * RU: Принять как styxrpc_named_recv, проверяя stop до отправки и между
 * опросами ответа. При CANCELLED закрыть file без повторного использования. */
StyxRpcIoOutcome styxrpc_named_recv_until(StyxRpcVfsFile *file, unsigned char **bytes, size_t *size,
    StyxRpcNamedStop stop, void *state);


/* EN: Send like styxrpc_named_send with the same cancellation protocol as recv_until.
 * A cancelled submitted send may have matched. Close file and never retry blindly.
 * RU: Послать как styxrpc_named_send с отменой по правилам recv_until.
 * Пара могла принять посылку. Закрыть file и не повторять отправку вслепую. */
StyxRpcIoOutcome styxrpc_named_send_until(StyxRpcVfsFile *file, const void *bytes, size_t size,
    StyxRpcNamedStop stop, void *state);

#ifdef __cplusplus
}
#endif
#endif
