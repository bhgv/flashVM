/* Copyright (c) 2026 bhgv
 * EN: Bounded, transport-independent state shared by the two message adapters.
 * RU: Ограниченное транспортно-независимое состояние двух адаптеров сообщений.
 */
#ifndef STYXRPC_MESSAGE_INTERNAL_H
#define STYXRPC_MESSAGE_INTERNAL_H
#include "styxrpc/messages.h"
#include "flashvm/sbindings.h"

#define MESSAGE_FRAME_MAX (STYXRPC_MESSAGE_PAYLOAD_MAX + 3U * FVM_SRECORD_INTEGER_MAX) /* Includes framing. */

typedef struct MessageRegistration
{
    FVM_SBinding binding;         /* Copied type name, version and optional send tag. */
    StyxRpcMessageHandler receive; /* Handler for incoming records of this schema. */
    void *context;                /* Borrowed handler state. */
} MessageRegistration;

typedef struct MessageConfig
{
    MessageRegistration types[STYXRPC_MESSAGE_TYPES_MAX]; /* Local type registrations. */
    size_t count;                                        /* Number of valid entries. */
} MessageConfig;

struct StyxRpcMessagePort
{
    const MessageConfig *config; /* Borrowed fixed local registry through disposal. */
    FVM_SBindings incoming;      /* Peer's direction-local tag table. */
    FVM_SBinding tags[STYXRPC_MESSAGE_TYPES_MAX]; /* Incoming registry storage. */
    unsigned char input[MESSAGE_FRAME_MAX];      /* Partial input and callback payload. */
    unsigned char output[2U * MESSAGE_FRAME_MAX]; /* Queued complete frames or their suffix. */
    size_t input_size;            /* Buffered input bytes. */
    size_t output_size;           /* Queued output bytes. */
    int bound;                    /* Peer has declared its outgoing tag bindings. */
    int failed;                   /* Terminal failure, retained until close. */
    int client;                   /* Owned client port when nonzero. */
};

typedef struct MessageService MessageService; /* Hosted registration and open-port count. */
extern const StyxRpcVfs styxrpc_message_files; /* Per-fid callbacks for the root node. */


/* EN: Copy and validate spec's type registrations into config, 1 on success.
 * RU: Скопировать регистрации типов spec в config и проверить, 1 при успехе. */
int styxrpc_message_config(MessageConfig *config, const StyxRpcMessageSpec *spec);


/* EN: Initialize zeroed port using config, with one queued tag-zero declaration.
 * RU: Настроить обнулённый port из config и поставить объявление с тегом ноль в очередь. */
int styxrpc_message_port_init(StyxRpcMessagePort *port, const MessageConfig *config);


/* EN: Append bytes and dispatch complete records on port. Set delivered on any user callback.
 * RU: Добавить байты и доставить полные записи port. Установить delivered при вызове обработчика. */
int styxrpc_message_feed(StyxRpcMessagePort *port, const void *bytes, size_t size, int *delivered);


/* EN: Remove size bytes already consumed from port's output (size <= output_size).
 * RU: Удалить size принятых байт из очереди port (size <= output_size). */
void styxrpc_message_drain(StyxRpcMessagePort *port, size_t size);


/* EN: Copy spec and verify no root name/identity collision in tree/context. Return owned service.
 * RU: Скопировать spec и проверить имя и ID в tree/context. Вернуть свой сервис. */
MessageService *styxrpc_message_service(const StyxRpcTree *tree, void *context,
    const StyxRpcNode *node, const StyxRpcMessageSpec *spec);

/* EN: Invalidate all attached streams, invoke their close hooks and release service.
 * Existing outer fids can still clunk. Call outside an exchange stream callback.
 * RU: Отозвать потоки, вызвать их хуки закрытия и освободить service.
 * Внешние fid по-прежнему можно закрыть. Вызывать вне колбэка потока обмена. */
void styxrpc_message_service_destroy(MessageService *service);

#endif
