/* Copyright (c) 2026 bhgv
 * EN: Allocation-free managed Dis RPC with an application-driven completion step.
 * RU: Управляемые RPC Dis без аллокатора с завершением по решению приложения.
 */
#ifndef STYXRPC_MANAGED_H
#define STYXRPC_MANAGED_H
#include "styxrpc/rpc.h"
#include "flashvm/srecord.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_MANAGED_SHAPE 49U /* Maximum schema string capacity including NUL. */
#define STYXRPC_MANAGED_LIMIT 65536U /* Protocol payload/frame ceiling, in bytes. */

/* EN: Tags for child records in the existing managed wire grammar.
 * RU: Теги дочерних записей в существующем управляемом формате. */
#define STYXRPC_VALUE_INT 1UL    /* Signed int using the established scalar encoding. */
#define STYXRPC_VALUE_BIG 2UL    /* Signed big using the established scalar encoding. */
#define STYXRPC_VALUE_TUPLE 4UL  /* Ordered encoded fields. */
#define STYXRPC_VALUE_REAL 5UL   /* Binary64 bits using the established scalar encoding. */
#define STYXRPC_VALUE_BYTE 6UL   /* Unsigned byte using the established scalar encoding. */
#define STYXRPC_VALUE_STRING 7UL /* Counted UTF-8 bytes. */
#define STYXRPC_VALUE_ARRAY 8UL  /* Ordered encoded elements. */
#define STYXRPC_VALUE_NIL 9UL    /* Empty payload. */
#define STYXRPC_VALUE_REF 10UL   /* Object index byte followed by fields on first appearance. */


/* EN: Bounded shape strings use the existing compiled Dis wire grammar.
 * RU: Ограниченные строки типов используют существующую грамматику Dis. */
typedef struct StyxRpcSchema
{
    char arguments[STYXRPC_MANAGED_SHAPE]; /* Parenthesized list of argument types. */
    char result[STYXRPC_MANAGED_SHAPE];    /* Zero or one item in a parenthesized list. */
} StyxRpcSchema;


/* EN: Borrow encoded output children and ref updates until completion returns.
 * RU: Заимствованные потомки результата и обновления ref до конца завершения. */
typedef struct StyxRpcOutput
{
    const void *result;   /* Encoded result children, without the outer S-record. */
    size_t result_size;   /* Byte extent of result. */
    const void *updates;  /* Encoded changed ref definitions, numbered densely from zero. */
    size_t update_size;   /* Byte extent of updates. */
    uint8_t changed;      /* Bit mask in flattened ref argument order. */
} StyxRpcOutput;


/* EN: Return OK to publish output, MORE to defer, or REMOTE_ERROR to refuse.
 * Arguments are checked, bound to this fid, and borrowed until close.
 * RU: Вернуть OK для готового результата, MORE для ожидания, REMOTE_ERROR для отказа.
 * Аргументы проверены, закреплены за fid и заимствуются до закрытия. */
typedef StyxRpcStatus (*StyxRpcBegin)(void *context, const FVM_SRecord *arguments, StyxRpcOutput *output);


/* EN: Application registration shared by every opened fid of one export.
 * RU: Описание приложения, общее для всех открытых fid одного экспорта. */
typedef struct StyxRpcManagedExport
{
    const char *name;       /* Borrowed exact Dis export name. */
    uint32_t signature;     /* Existing compiled Dis signature. */
    StyxRpcSchema schema;   /* Resolved wire types of the compiled export. */
    StyxRpcBegin begin;     /* Called once per accepted frame. */
    void (*cancel)(void *context, const FVM_SRecord *arguments); /* Optional pending-call cleanup. */
    void *context;          /* Application data shared across opens. */
} StyxRpcManagedExport;


/* EN: An application owns all storage, with separate buffers for each fid.
 * RU: У приложения отдельное состояние и буферы для каждого fid. */
typedef struct StyxRpcManagedCall
{
    const StyxRpcManagedExport *exported; /* Borrowed until close. */
    unsigned char *request;               /* Writable request storage, immutable once accepted. */
    unsigned char *reply;                 /* Writable reply storage, immutable once complete. */
    size_t request_capacity;              /* Bytes available for the complete request. */
    size_t reply_capacity;                /* Bytes available for the complete reply. */
    size_t staged;                        /* Acknowledged request prefix length. */
    size_t ready;                         /* Published reply length. */
    FVM_SRecord arguments;                /* Checked view into request. */
    int pending;                          /* Begin has accepted the call. */
    int refused;                          /* Sticky failure until clunk. */
} StyxRpcManagedCall;


/* EN: Encode signed int, signed big, or unsigned byte as a managed child.
 * RU: Закодировать int, big или беззнаковый byte как дочернюю запись. */
StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value, void *bytes,
    size_t capacity, size_t *used);


/* EN: Write the single export's compatible shaped .interface entry into bytes.
 * RU: Записать совместимый типизированный экспорт .interface в bytes. */
StyxRpcStatus styxrpc_managed_profile(const StyxRpcManagedExport *exported,
    void *bytes, size_t capacity, size_t *size);


/* EN: Initialize fresh/closed call with disjoint application buffers of 1..LIMIT bytes.
 * RU: Инициализировать новое/закрытое состояние непересекающимися буферами от 1 до LIMIT байт. */
StyxRpcStatus styxrpc_managed_open(StyxRpcManagedCall *call, const StyxRpcManagedExport *exported,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity);


/* EN: Stage bytes at the next offset, then run begin once when the frame is complete.
 * Scratch is borrowed until return and must be disjoint from input, output and fid buffers.
 * RU: Принять очередные байты и один раз вызвать begin после полного кадра.
 * Scratch заимствуется до возврата и не пересекается с остальными буферами. */
StyxRpcStatus styxrpc_managed_write(StyxRpcManagedCall *call, size_t offset, const void *bytes, size_t size,
    void *scratch, size_t capacity);


/* EN: Finish a pending call with success (output) or a remote error (NULL).
 * Output and scratch must be disjoint from the reply and from each other.
 * RU: Завершить ожидающий вызов результатом output либо ошибкой NULL.
 * Выходные данные, scratch и буфер ответа не пересекаются. */
StyxRpcStatus styxrpc_managed_finish(StyxRpcManagedCall *call, const StyxRpcOutput *output,
    void *scratch, size_t capacity);


/* EN: Read the reply by byte offset. MORE while waiting, OK/zero at EOF.
 * RU: Читать ответ по байтовому смещению. MORE при ожидании, OK/ноль в EOF. */
StyxRpcStatus styxrpc_managed_read(const StyxRpcManagedCall *call, size_t offset,
    void *bytes, size_t capacity, size_t *size);


/* EN: Cancel one pending call at clunk or disconnect. Serialize all operations on a fid.
 * RU: Отменить ожидающий вызов при clunk/обрыве. Операции одного fid сериализуются. */
void styxrpc_managed_close(StyxRpcManagedCall *call);

/* EN: Checked borrowed reply views into the caller's encoded reply buffer.
 * RU: Проверенные заимствованные представления из буфера ответа. */
typedef struct StyxRpcManagedReply
{
    FVM_SRecord result;  /* List of return values, empty struct when void_result is true. */
    FVM_SRecord updates; /* Selected new ref records, in argument order. */
    unsigned changed;    /* Bit mask in flattened ref argument order. */
    int void_result;     /* True when the result field was nil for void. */
} StyxRpcManagedReply;


/* EN: Validate argument children and seal them in a managed request.
 * Payload and bytes must be disjoint. On failure used is zero.
 * RU: Проверить дочерние записи аргументов и сформировать запрос.
 * Payload и bytes не пересекаются. При отказе used равен нулю. */
StyxRpcStatus styxrpc_managed_request(uint32_t signature, const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used);


/* EN: Validate a whole reply and update mask against the exact original request.
 * Publish borrowed views only on success; the application commits ref changes.
 * RU: Проверить полный ответ и маску изменений по исходному запросу.
 * Представления выдавать при успехе; изменения ref применяет приложение. */
StyxRpcStatus styxrpc_managed_reply(uint32_t signature, const StyxRpcSchema *schema,
    const void *request, size_t request_size, const void *reply, size_t reply_size, StyxRpcManagedReply *result);

#ifdef __cplusplus
}
#endif
#endif
