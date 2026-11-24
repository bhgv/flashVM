/* Copyright (c) 2026 bhgv
 * EN: Fixed custom type registration, streaming S-record verification and dispatch.
 * RU: Реестр пользовательских типов, потоковая проверка и доставка S-записей.
 */
#include "message_internal.h"
#include <string.h>


/* EN: Find an exact local name/version in config, or return NULL.
 * RU: Найти точное имя и версию в config либо вернуть NULL. */
static const MessageRegistration *find_type(const MessageConfig *config, const char *name, uint32_t version)
{
    for (size_t i = 0; i < config->count; ++i)
    {
        const MessageRegistration *entry = &config->types[i];
        if (entry->binding.version == version && !strcmp(entry->binding.type, name))
        {
            return entry;
        }
    }
    return NULL;
}


/* EN: Check a NUL-terminated schema name and return its length, zero for an invalid name.
 * RU: Проверить имя схемы с NUL и вернуть длину, ноль для недопустимого имени. */
static size_t type_length(const char *name)
{
    if (!name)
    {
        return 0;
    }
    for (size_t i = 0; i <= STYXRPC_MESSAGE_NAME_MAX; ++i)
    {
        unsigned char c = (unsigned char)name[i];
        if (!c)
        {
            return i;
        }
        if (c < 0x21U || c > 0x7eU)
        {
            break;
        }
    }
    return 0;
}


/* EN/RU: See message_internal.h. / См. message_internal.h. */
int styxrpc_message_config(MessageConfig *config, const StyxRpcMessageSpec *spec)
{
    if (!spec || !spec->types || !spec->count || spec->count > STYXRPC_MESSAGE_TYPES_MAX)
    {
        return 0;
    }
    config->count = 0;
    for (size_t i = 0; i < spec->count; ++i)
    {
        const StyxRpcMessageType *type = &spec->types[i];
        size_t length = type_length(type->name);
        if (!length || !type->version || (!type->send_tag && !type->receive) ||
            find_type(config, type->name, type->version))
        {
            return 0;
        }
        for (size_t j = 0; type->send_tag && j < i; ++j)
        {
            if (config->types[j].binding.tag == type->send_tag)
            {
                return 0;
            }
        }
        MessageRegistration *entry = &config->types[config->count++];
        *entry = (MessageRegistration){.binding = {.tag = type->send_tag,
            .version = type->version, .type_size = (unsigned int)length},
            .receive = type->receive, .context = type->context};
        memcpy(entry->binding.type, type->name, length + 1);
    }
    return 1;
}


/* EN/RU: See message_internal.h. / См. message_internal.h. */
int styxrpc_message_port_init(StyxRpcMessagePort *port, const MessageConfig *config)
{
    FVM_SBinding outgoing[STYXRPC_MESSAGE_TYPES_MAX];
    size_t count = 0;
    for (size_t i = 0; i < config->count; ++i)
    {
        if (config->types[i].binding.tag)
        {
            outgoing[count++] = config->types[i].binding;
        }
    }
    unsigned char declaration[(2U + 3U * STYXRPC_MESSAGE_TYPES_MAX) * FVM_SRECORD_INTEGER_MAX +
        STYXRPC_MESSAGE_TYPES_MAX * STYXRPC_MESSAGE_NAME_MAX];
    size_t size;
    port->config = config;
    return fvm_sbindings_init(&port->incoming, port->tags, STYXRPC_MESSAGE_TYPES_MAX) == FVM_S_OK &&
        fvm_sbindings_payload_write(outgoing, count, declaration, sizeof(declaration), &size) == FVM_S_OK &&
        fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, declaration, size, port->output,
            sizeof(port->output), &port->output_size) == FVM_S_OK;
}


/* EN/RU: See messages.h. / См. messages.h. */
StyxRpcStatus styxrpc_message_send(StyxRpcMessagePort *port, const char *name, uint32_t version,
    const void *payload, size_t size)
{
    if (!port || port->failed || !name || (!payload && size) || size > STYXRPC_MESSAGE_PAYLOAD_MAX)
    {
        return STYXRPC_INVALID;
    }
    const MessageRegistration *entry = find_type(port->config, name, version);
    if (!entry || !entry->binding.tag)
    {
        return STYXRPC_INVALID;
    }
    size_t written;
    FVM_SResult status = fvm_srecord_write(entry->binding.tag, payload, size,
        port->output + port->output_size, sizeof(port->output) - port->output_size, &written);
    if (status != FVM_S_OK)
    {
        return status == FVM_S_LIMIT ? STYXRPC_MORE : STYXRPC_INVALID;
    }
    port->output_size += written;
    return STYXRPC_OK;
}


/* EN: Apply a declaration from bytes or dispatch a checked record from port. Set delivered for user data.
 * RU: Применить объявление из bytes или доставить запись port. Установить delivered для данных приложения. */
static int accept_record(StyxRpcMessagePort *port, const unsigned char *bytes,
    const FVM_SRecord *record, int *delivered)
{
    if (record->tag == FVM_SRECORD_CONTROL_TAG)
    {
        size_t used;
        if (port->bound || fvm_sbindings_apply(&port->incoming, bytes,
            record->encoded_size, &used) != FVM_S_OK)
        {
            return 0;
        }
        port->bound = 1;
        return 1;
    }
    FVM_SBinding binding;
    if (!port->bound || record->size > STYXRPC_MESSAGE_PAYLOAD_MAX ||
        fvm_sbindings_lookup(&port->incoming, record->tag, &binding) != FVM_S_OK)
    {
        return 0;
    }
    const MessageRegistration *entry = find_type(port->config, binding.type, (uint32_t)binding.version);
    if (!entry || !entry->receive || entry->receive(port, entry->context, record->payload, record->size) != 1)
    {
        return 0;
    }
    *delivered = 1;
    return 1;
}


/* EN/RU: See message_internal.h. / См. message_internal.h. */
int styxrpc_message_feed(StyxRpcMessagePort *port, const void *bytes, size_t size, int *delivered)
{
    const unsigned char *next = bytes;
    while (size)
    {
        size_t available = sizeof(port->input) - port->input_size;
        size_t chunk = size < available ? size : available;
        memcpy(port->input + port->input_size, next, chunk);
        port->input_size += chunk;
        next += chunk;
        size -= chunk;

        size_t offset = 0;
        while (offset < port->input_size)
        {
            const unsigned char *record_bytes = port->input + offset;
            FVM_SRecord record;
            size_t consumed;
            FVM_SResult status = fvm_srecord_read(record_bytes, port->input_size - offset,
                STYXRPC_MESSAGE_PAYLOAD_MAX + FVM_SRECORD_INTEGER_MAX, &record, &consumed);
            if (status == FVM_S_MORE && (offset || port->input_size < sizeof(port->input)))
            {
                break;
            }
            if (status != FVM_S_OK || !accept_record(port, record_bytes, &record, delivered))
            {
                port->failed = 1;
                return 0;
            }
            offset += consumed;
        }
        if (offset)
        {
            port->input_size -= offset;
            memmove(port->input, port->input + offset, port->input_size);
        }
    }
    return 1;
}


/* EN/RU: See message_internal.h. / См. message_internal.h. */
void styxrpc_message_drain(StyxRpcMessagePort *port, size_t size)
{
    port->output_size -= size;
    memmove(port->output, port->output + size, port->output_size);
}
