/* Copyright (c) 2026 bhgv
 * EN: Send and receive mixed ActionScript values on one named UniBus port.
 * RU: Отправка и приём разных значений ActionScript через один именованный порт UniBus.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "variable_ref_adapter.h"
#include "flashvm/userdata.h"
#include "styxrpc/channel_messages.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_SCALAR_BUFFER (3U * FVM_SRECORD_INTEGER_MAX + STYXRPC_INT_REF_SIZE) /* Numeric or ref wire scratch. */

typedef enum ChannelKind
{
    CHANNEL_INT,  /* A signed 32-bit integer snapshot. */
    CHANNEL_REAL, /* A binary64 snapshot. */
    CHANNEL_BOOL, /* A Boolean snapshot. */
    CHANNEL_TEXT, /* A counted AS string snapshot. */
    CHANNEL_NULL, /* The null value. */
    CHANNEL_REF,  /* The original issuer's live Int32 reference. */
    CHANNEL_KINDS /* Size of the local schema table. */
} ChannelKind;

static const char *const channel_names[CHANNEL_KINDS] = {
    STYXRPC_CHANNEL_MESSAGE_INT, STYXRPC_CHANNEL_MESSAGE_REAL, STYXRPC_CHANNEL_MESSAGE_BOOL,
    STYXRPC_CHANNEL_MESSAGE_TEXT, STYXRPC_CHANNEL_MESSAGE_NULL, STYXRPC_CHANNEL_MESSAGE_REF
}; /* Exact UniBus identities in local kind order. */

typedef struct ScriptChannel ScriptChannel; /* Channel state retained by userdata. */

typedef struct ChannelType
{
    ScriptChannel *channel; /* Borrowed through disposal of the SDK port. */
    ChannelKind kind;       /* Local codec matched by the SDK to a peer tag. */
} ChannelType;

typedef struct ChannelRecord
{
    struct ChannelRecord *next; /* Owned next record, in wire order. */
    ChannelKind kind;           /* Codec for the trailing bytes. */
    size_t size;                /* Length of the trailing bytes. */
} ChannelRecord;

struct ScriptChannel
{
    FVM_StyxRpcSession *session;     /* Host binding, valid through context destruction. */
    StyxRpcMessagePort *port;       /* Owned exchange fid and protocol buffers. */
    ChannelType types[CHANNEL_KINDS]; /* Callback contexts copied by reference into the port. */
    ChannelRecord *head;            /* Undelivered records from at most one pump. */
    ChannelRecord **tail;          /* Address of the next FIFO link to fill. */
};


/* EN: Release raw records in channel without running script or performing I/O.
 * RU: Освободить записи channel без вызова скрипта и без I/O. */
static void clear_channel(ScriptChannel *channel)
{
    while (channel->head)
    {
        ChannelRecord *record = channel->head;
        channel->head = record->next;
        free(record);
    }
    channel->tail = &channel->head;
}


/* [GC:] EN: Release userdata state, deferring any remaining fid close.
 * RU: Освободить состояние userdata, отложив закрытие оставшегося fid. */
static void release_channel(void *state)
{
    ScriptChannel *channel = state;
    styxrpc_messages_abandon(channel->port);
    clear_channel(channel);
    free(channel);
}


/* EN: Copy size bytes into the current channel pump queue. Allocation failure retires port.
 * RU: Скопировать size байт в очередь текущего приёма. Ошибка выделения закрывает port. */
static int receive_record(StyxRpcMessagePort *port, void *state, const void *bytes, size_t size)
{
    ChannelType *type = state;
    ChannelRecord *record = malloc(sizeof(*record) + size);
    (void)port;
    if (!record)
    {
        return 0;
    }
    record->next = NULL;
    record->kind = type->kind;
    record->size = size;
    if (size)
    {
        memcpy(record + 1, bytes, size);
    }
    *type->channel->tail = record;
    type->channel->tail = &record->next;
    return 1;
}


/* EN: Queue a snapshot or imported reference operand on channel. The SDK copies the bytes.
 * RU: Поставить в очередь channel снимок значения или импортированную ссылку. SDK копирует байты. */
static StyxRpcStatus queue_value(ScriptChannel *channel, const FVM_Value *value)
{
    unsigned char scratch[CHANNEL_SCALAR_BUFFER];
    const void *bytes = scratch;
    size_t size = 0;
    ChannelKind kind;
    const double positive_zero = 0.0;
    switch (value->type)
    {
    case FVM_VALUE_NUMBER:
        if (value->number >= INT32_MIN && value->number <= INT32_MAX &&
            (double)(int32_t)value->number == value->number &&
            (value->number != 0 || !memcmp(&value->number, &positive_zero, sizeof(positive_zero))))
        {
            kind = CHANNEL_INT;
            if (styxrpc_value_integer(STYXRPC_VALUE_INT, (int32_t)value->number, scratch,
                sizeof(scratch), &size) != STYXRPC_OK)
            {
                return STYXRPC_INVALID;
            }
        }
        else
        {
            kind = CHANNEL_REAL;
            if (styxrpc_value_real(value->number, scratch, sizeof(scratch), &size) != STYXRPC_OK)
            {
                return STYXRPC_INVALID;
            }
        }
        break;
    case FVM_VALUE_BOOLEAN:
        kind = CHANNEL_BOOL;
        scratch[0] = value->boolean != 0;
        size = 1;
        break;
    case FVM_VALUE_STRING:
        kind = CHANNEL_TEXT;
        bytes = value->string;
        size = fvm_value_string_size(value);
        break;
    case FVM_VALUE_NULL:
        kind = CHANNEL_NULL;
        break;
    case FVM_VALUE_OBJECT:
        kind = CHANNEL_REF;
        if (!fvm_styxrpc_encode_int_ref(channel->session, value, scratch, sizeof(scratch)))
        {
            return STYXRPC_INVALID;
        }
        size = STYXRPC_INT_REF_SIZE;
        break;
    default:
        return STYXRPC_INVALID;
    }
    return styxrpc_message_send(channel->port, channel_names[kind], STYXRPC_CHANNEL_MESSAGE_VERSION, bytes, size);
}


/* [GC:] EN: Materialize a counted wire string as a context-owned AS string in out.
 * RU: Создать в out строку AS во владении контекста из size сетевых байт. */
static FVM_Result decode_text(FVM_Context *context, const unsigned char *bytes, size_t size, FVM_Value *out)
{
    FVM_Value borrowed;
    size_t count;
    unsigned short *units = malloc((size ? size : 1) * sizeof(*units));
    FVM_Result status;
    if (!units)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_value_string_bytes(&borrowed, (const char *)bytes, size);
    if (status == FVM_OK)
    {
        status = fvm_value_get_utf16(&borrowed, units, size, &count);
    }
    if (status == FVM_OK)
    {
        status = fvm_value_string_utf16(context, units, count, out);
    }
    free(units);
    return status;
}


/* [ASINC] [GC:] EN: Decode one retained record. A reference opens its issuer fid after the pump has ended.
 * RU: Декодировать одну удерживаемую запись. Ссылка открывает fid владельца после завершения приёма. */
static FVM_Result decode_record(FVM_Context *context, ScriptChannel *channel, const ChannelRecord *record,
    FVM_Value *out)
{
    const unsigned char *bytes = (const unsigned char *)(record + 1);
    int64_t integer;
    double real;
    switch (record->kind)
    {
    case CHANNEL_INT:
        if (styxrpc_value_get_integer(STYXRPC_VALUE_INT, bytes, record->size, &integer) == STYXRPC_OK)
        {
            fvm_value_number(out, (double)integer);
            return FVM_OK;
        }
        break;
    case CHANNEL_REAL:
        if (styxrpc_value_get_real(bytes, record->size, &real) == STYXRPC_OK)
        {
            fvm_value_number(out, real);
            return FVM_OK;
        }
        break;
    case CHANNEL_BOOL:
        if (record->size == 1 && bytes[0] <= 1)
        {
            fvm_value_boolean(out, bytes[0]);
            return FVM_OK;
        }
        break;
    case CHANNEL_NULL:
        if (!record->size)
        {
            fvm_value_undefined(out);
            out->type = FVM_VALUE_NULL;
            return FVM_OK;
        }
        break;
    case CHANNEL_TEXT:
        return decode_text(context, bytes, record->size, out);
    case CHANNEL_REF:
        return fvm_styxrpc_decode_int_ref(context, channel->session, bytes, record->size, out);
    default:
        break;
    }
    return FVM_INVALID_ARGUMENT;
}


/* EN: Discard retained input and retire the unusable channel fid without further I/O.
 * RU: Удалить принятые данные и вывести непригодный fid канала из работы без I/O. */
static void abandon_channel(ScriptChannel *channel)
{
    styxrpc_messages_abandon(channel->port);
    channel->port = NULL;
    clear_channel(channel);
}


/* [ASINC] EN: Submit args[0] to the peer on this channel. True confirms the Styx write.
 * RU: Отправить args[0] другой стороне канала. True подтверждает запись Styx. */
static FVM_Result send_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    (void)self;
    if (!channel->session->client || !channel->port || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (queue_value(channel, args) != STYXRPC_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (styxrpc_messages_flush(channel->port) != STYXRPC_OK)
    {
        abandon_channel(channel);
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel write failed");
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Return one FIFO value, or undefined if one bounded transport poll yields none.
 * RU: Вернуть одно значение FIFO или undefined, если один ограниченный опрос ничего не дал. */
static FVM_Result recv_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    ChannelRecord *record;
    FVM_Result status;
    (void)self;
    (void)args;
    if (!channel->session->client || !channel->port || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (!channel->head && styxrpc_messages_pump(channel->port) == STYXRPC_INVALID)
    {
        abandon_channel(channel);
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel read failed");
    }
    record = channel->head;
    if (!record)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    channel->head = record->next;
    if (!channel->head)
    {
        channel->tail = &channel->head;
    }
    status = decode_record(context, channel, record, out);
    free(record);
    return status;
}


/* [ASINC] EN: Close the fid and release buffered records. Repeated close succeeds.
 * RU: Закрыть fid и освободить накопленные записи. Повторное закрытие успешно. */
static FVM_Result close_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (channel->session->client)
    {
        styxrpc_messages_close(channel->port);
        channel->port = NULL;
        clear_channel(channel);
    }
    else
    {
        abandon_channel(channel);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Attach a named peer exchange root and retain its port under the AS object.
 * RU: Подключить именованный корень обмена другой стороны и сохранить порт под объектом AS. */
static FVM_Result new_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    char address[STYXRPC_TREE_NAME + 2];
    const char *name;
    FVM_GCRootScope *roots;
    FVM_Result status;
    FVM_Value object;
    ScriptChannel *channel = NULL;
    (void)self;
    if (!binding->client || argc != 1 || !fvm_styxrpc_name(args, address, sizeof(address)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    name = address + (address[0] == '/');
    if (!*name || strchr(name, '/') || strlen(name) > STYXRPC_TREE_NAME)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    if (status == FVM_OK)
    {
        channel = calloc(1, sizeof(*channel));
        status = channel ? fvm_userdata_attach(context, object.object, channel, release_channel) : FVM_OUT_OF_MEMORY;
        if (status != FVM_OK)
        {
            free(channel);
        }
    }
    if (status == FVM_OK)
    {
        channel->session = binding;
        channel->tail = &channel->head;
        status = fvm_object_define_method_rooted(context, object.object, "send", send_value, channel, object.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "recv", recv_value, channel, object.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "close", close_channel, channel, object.object);
    }
    if (status == FVM_OK)
    {
        StyxRpcMessageType types[CHANNEL_KINDS];
        StyxRpcMessageSpec spec = {0};
        unsigned i;
        for (i = 0; i < CHANNEL_KINDS; ++i)
        {
            channel->types[i].channel = channel;
            channel->types[i].kind = (ChannelKind)i;
            types[i].name = channel_names[i];
            types[i].version = STYXRPC_CHANNEL_MESSAGE_VERSION;
            types[i].send_tag = i + 1U;
            types[i].receive = receive_record;
            types[i].context = &channel->types[i];
        }
        spec.types = types;
        spec.count = CHANNEL_KINDS;
        if (styxrpc_messages_open_at(binding->client, name, &spec, &channel->port) != STYXRPC_OK)
        {
            status = FVM_NOT_FOUND;
        }
    }
    if (status == FVM_OK)
    {
        *out = object;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_channel_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    return fvm_object_define_method_rooted(context, object, "newchan", new_channel, binding, NULL);
}
