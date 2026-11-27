/* Copyright (c) 2026 bhgv
 * EN: Client operations on exact live variable trees published over Styx.
 * RU: Клиентские операции над точными деревьями живых переменных Styx.
 */
#ifndef STYXRPC_LIVE_CLIENT_H
#define STYXRPC_LIVE_CLIENT_H
#include "styxrpc/live_variable.h"
#include "styxrpc/vfs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcLive StyxRpcLive; /* Owned exact unopened fid and path identity. */
typedef enum StyxRpcLiveKind
{
    STYXRPC_LIVE_INT,     /* Signed four-byte scalar file. */
    STYXRPC_LIVE_INTS,    /* Fixed array of Int32 cells. */
    STYXRPC_LIVE_BIGS,    /* Indexed vector or numeric half of a hybrid table. */
    STYXRPC_LIVE_ROW,     /* Named field directory. */
    STYXRPC_LIVE_TEXT,    /* Counted string or nil. */
    STYXRPC_LIVE_BIG      /* Exact signed 64-bit scalar file in a typed directory. */
} StyxRpcLiveKind;

typedef struct StyxRpcLiveValue
{
    int64_t integer;                   /* Signed scalar on numeric proxies. */
    size_t size;                       /* Number of UTF-8 bytes for a string. */
    int nil;                           /* 1 for a nil String, 0 for a String with size bytes. */
    char text[STYX_LIVE_STRING_LIMIT]; /* Counted bytes, with embedded zero permitted. */
} StyxRpcLiveValue;


/* EN: Capture one canonical relative path on session. Borrow stop(state)
 * throughout the walk and type check. Writable is 0 or 1. Publish out on success.
 * RU: Захватить канонический относительный путь в session с проверкой stop(state).
 * Writable равен 0 или 1. Выдать собственный out после определения типа. */
StyxRpcIoOutcome styxrpc_live_open_until(struct StyxClient *session, const char *path, int writable,
    StyxRpcLive **out, StyxRpcVfsStop stop, void *state);


/* EN: Read immutable local type information from a valid borrowed binding.
 * RU: Прочитать неизменяемый локальный тип действительной заимствованной привязки. */
StyxRpcLiveKind styxrpc_live_kind(const StyxRpcLive *live);


/* EN: Capture one named field on a record. Inherit the parent's access ceiling.
 * Publish an independently owned binding under out on success.
 * RU: Захватить поле записи с ограничением прав родителя. При успехе
 * передать независимо владеющую привязку в out. */
StyxRpcIoOutcome styxrpc_live_field_until(const StyxRpcLive *live, const char *name, StyxRpcLive **out,
    StyxRpcVfsStop stop, void *state);


/* EN: Check the current identity and read the element or field count into length.
 * RU: Проверить текущую идентичность и прочитать число элементов или полей в length. */
StyxRpcIoOutcome styxrpc_live_length_until(const StyxRpcLive *live, uint32_t *length, StyxRpcVfsStop stop, void *state);


/* EN: Read one live value; index is zero for a scalar or string. Preserve out on failure.
 * RU: Прочитать живое значение; для скаляра или строки index равен нулю. Сохранить out при отказе. */
StyxRpcIoOutcome styxrpc_live_read_until(const StyxRpcLive *live, int64_t index, StyxRpcLiveValue *out,
    StyxRpcVfsStop stop, void *state);


/* EN: Replace one live value; index is zero for a scalar or string. An uncertain
 * write may have taken effect and is never replayed. Serialize operations per binding.
 * RU: Заменить живое значение; для скаляра или строки index равен нулю.
 * Неподтверждённая запись могла изменить данные, повтора нет. Вызовы сериализуются. */
StyxRpcIoOutcome styxrpc_live_write_until(const StyxRpcLive *live, int64_t index, const StyxRpcLiveValue *value,
    StyxRpcVfsStop stop, void *state);


/* EN: Consume a binding (NULL permitted); defer clunk when deferred is nonzero.
 * RU: Поглотить привязку (допустим NULL); отложить clunk при ненулевом deferred. */
void styxrpc_live_close(StyxRpcLive *live, int deferred);

#ifdef __cplusplus
}
#endif
#endif
