/* Copyright (c) 2026 bhgv
 * EN: Validated managed records and per-fid callback lifecycle.
 * RU: Проверенные управляемые записи и жизненный цикл колбэка на fid.
 */
#include "styxrpc/managed.h"
#include "call_record.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>


/* EN: Check the bounded portable shape against the compiled signature.
 * RU: Сверить ограниченную переносимую схему со скомпилированной сигнатурой. */
static int shape_for(uint32_t signature, const StyxRpcSchema *schema, StyxCallShape *shape)
{
    if (!schema || !memchr(schema->arguments, 0, sizeof(schema->arguments)) ||
        !memchr(schema->result, 0, sizeof(schema->result)))
    {
        return 0;
    }
    memcpy(shape->arguments, schema->arguments, sizeof(shape->arguments));
    memcpy(shape->result, schema->result, sizeof(shape->result));
    return styxshaperequest(signature, shape) == STYX_MANAGED_REQUEST_TAG &&
        !styxmanaged_graph_required(shape->arguments) &&
        !strpbrk(shape->arguments, "ch") && !strpbrk(shape->result, "chr");
}


/* EN: Check one callable export and retain its validated local shape.
 * RU: Проверить вызываемый экспорт и сохранить локальную форму. */
static int schema_for(const StyxRpcManagedExport *exported, StyxCallShape *shape)
{
    return exported && exported->begin && styxscalar_name_length(exported->name) &&
        exported->name[0] != '.' && !strchr(exported->name, '/') &&
        shape_for(exported->signature, &exported->schema, shape);
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_value_integer(unsigned long tag, int64_t value, void *bytes,
    size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    int valid = tag == STYXRPC_VALUE_BIG ||
        (tag == STYXRPC_VALUE_INT && value >= INT32_MIN && value <= INT32_MAX) ||
        (tag == STYXRPC_VALUE_BYTE && value >= 0 && value <= UINT8_MAX);
    return valid && bytes && styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes, capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_profile(const StyxRpcManagedExport *exported,
    void *bytes, size_t capacity, size_t *size)
{
    StyxCallShape shape;
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    return bytes && schema_for(exported, &shape) &&
        styxshapedoffer(exported->name, exported->signature, &shape, bytes, capacity, size) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_open(StyxRpcManagedCall *call, const StyxRpcManagedExport *exported,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity)
{
    StyxCallShape shape;
    if (!call || !request || !reply || !request_capacity || !reply_capacity ||
        request_capacity > STYXRPC_MANAGED_LIMIT || reply_capacity > STYXRPC_MANAGED_LIMIT ||
        !schema_for(exported, &shape))
    {
        return STYXRPC_INVALID;
    }
    *call = (StyxRpcManagedCall){.exported = exported, .request = request, .reply = reply,
        .request_capacity = request_capacity, .reply_capacity = reply_capacity};
    return STYXRPC_OK;
}


/* EN: Ensure every selected ref is a real defining argument and updates match its schema.
 * RU: Проверить, что выбраны исходные определения ref, а изменения совпадают со схемой. */
static int updates_valid(const char *shape, const FVM_SRecord *arguments, const void *bytes, size_t size,
    unsigned *mask, FVM_SRecord *updates)
{
    char selected[STYX_SHAPE_MAX + 1];
    unsigned confirmed;
    if (!styxmanaged_updates(shape, bytes, size, selected, &confirmed, updates))
    {
        return 0;
    }
    if (confirmed)
    {
        FVM_SRecord originals[STYX_CALL_VALUES];
        size_t count;
        if (!styxmanaged_ref_records(shape, arguments, originals, &count) || (confirmed >> count))
        {
            return 0;
        }
        for (size_t i = 0; i < count; ++i)
        {
            if ((confirmed & (1U << i)) && (originals[i].tag != STYX_MANAGED_REF_TAG || originals[i].size < 2))
            {
                return 0;
            }
        }
    }
    *mask = confirmed;
    return 1;
}


/* EN: Encode checked reply children in disjoint scratch, then publish both fields atomically.
 * RU: Записать проверенные поля ответа в отдельный scratch, выдать только вместе. */
static int seal(StyxRpcManagedCall *call, const StyxRpcOutput *output, unsigned char *scratch, size_t capacity)
{
    size_t result_size, update_size;
    const char *result = call->exported->schema.result;
    if (!scratch || (output->result_size && !output->result) ||
        (output->update_size && !output->updates) || (!output->changed && output->update_size))
    {
        return 0;
    }
    int valid = !strcmp(result, "()") ?
        !output->result_size && fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0,
            scratch, capacity, &result_size) == FVM_S_OK :
        styxmanaged_list_write(STYX_DYNAMIC_REPLY_TAG, result, output->result, output->result_size,
            scratch, capacity, &result_size);
    FVM_SRecord updates;
    unsigned changed;
    if (!valid || fvm_srecord_write_pair(STYX_MANAGED_ARRAY_TAG, &output->changed, output->changed ? 1 : 0,
        output->updates, output->update_size, scratch + result_size, capacity - result_size,
        &update_size) != FVM_S_OK ||
        !updates_valid(call->exported->schema.arguments, &call->arguments, scratch + result_size, update_size,
            &changed, &updates) || changed != output->changed)
    {
        return 0;
    }
    return styxcall_reply_write(STYX_MANAGED_REPLY_TAG, scratch, result_size,
        scratch + result_size, update_size, call->reply, call->reply_capacity, &call->ready);
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_finish(StyxRpcManagedCall *call, const StyxRpcOutput *output,
    void *scratch, size_t capacity)
{
    if (!call || !call->pending)
    {
        return STYXRPC_INVALID;
    }
    call->pending = 0;
    if (output && seal(call, output, scratch, capacity))
    {
        return STYXRPC_OK;
    }
    if (styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0,
        call->reply, call->reply_capacity, &call->ready))
    {
        return STYXRPC_REMOTE_ERROR;
    }
    call->refused = 1;
    return STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_write(StyxRpcManagedCall *call, size_t offset, const void *bytes, size_t size,
    void *scratch, size_t capacity)
{
    if (!call || !call->exported || call->ready || call->pending || call->refused || !bytes || !size ||
        offset != call->staged || size > call->request_capacity - call->staged)
    {
        return STYXRPC_INVALID;
    }
    memcpy(call->request + call->staged, bytes, size);
    call->staged += size;
    FVM_SRecord frame;
    size_t used;
    FVM_SResult parsed = fvm_srecord_read(call->request, call->staged, call->request_capacity, &frame, &used);
    if (parsed == FVM_S_MORE && call->staged < call->request_capacity)
    {
        return STYXRPC_MORE;
    }
    if (parsed != FVM_S_OK || used != call->staged || frame.tag != STYX_MANAGED_REQUEST_TAG ||
        !styxmanaged_request_payload(call->exported->schema.arguments, &frame))
    {
        call->refused = 1;
        return STYXRPC_INVALID;
    }
    call->arguments = frame;
    call->pending = 1;
    StyxRpcOutput output = {0};
    StyxRpcStatus status = call->exported->begin(call->exported->context, &call->arguments, &output);
    if (status == STYXRPC_MORE)
    {
        return STYXRPC_MORE;
    }
    if (status == STYXRPC_OK)
    {
        return styxrpc_managed_finish(call, &output, scratch, capacity);
    }
    return styxrpc_managed_finish(call, NULL, NULL, 0);
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_read(const StyxRpcManagedCall *call, size_t offset,
    void *bytes, size_t capacity, size_t *size)
{
    if (!size)
    {
        return STYXRPC_INVALID;
    }
    *size = 0;
    if (!call || !call->exported || call->refused || (!bytes && capacity))
    {
        return STYXRPC_INVALID;
    }
    if (!call->ready)
    {
        return STYXRPC_MORE;
    }
    if (offset < call->ready && capacity)
    {
        size_t available = call->ready - offset;
        *size = available < capacity ? available : capacity;
        memcpy(bytes, call->reply + offset, *size);
    }
    return STYXRPC_OK;
}


/* EN/RU: See managed.h. / См. managed.h. */
void styxrpc_managed_close(StyxRpcManagedCall *call)
{
    if (call && call->exported)
    {
        const StyxRpcManagedExport *exported = call->exported;
        call->exported = NULL;
        int pending = call->pending;
        call->pending = 0;
        if (pending && exported->cancel)
        {
            exported->cancel(exported->context, &call->arguments);
        }
    }
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_request(uint32_t signature, const StyxRpcSchema *schema, const void *payload,
    size_t size, void *bytes, size_t capacity, size_t *used)
{
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    StyxCallShape shape;
    return bytes && (payload || !size) && shape_for(signature, schema, &shape) &&
        styxmanaged_list_write(STYX_MANAGED_REQUEST_TAG, shape.arguments, payload, size, bytes, capacity, used) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See managed.h. / См. managed.h. */
StyxRpcStatus styxrpc_managed_reply(uint32_t signature, const StyxRpcSchema *schema,
    const void *request, size_t request_size, const void *reply, size_t reply_size, StyxRpcManagedReply *result)
{
    StyxCallShape shape;
    FVM_SRecord arguments;
    if (!result || !shape_for(signature, schema, &shape) ||
        !styxmanaged_list_read(STYX_MANAGED_REQUEST_TAG, shape.arguments, request, request_size, &arguments))
    {
        return STYXRPC_INVALID;
    }
    StyxScalarKind kind;
    int64_t code;
    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, reply, reply_size, &kind, &code) && kind == STYX_SCALAR_ERROR)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    StyxReplyField fields[2];
    StyxRpcManagedReply decoded = {0};
    const int empty = !strcmp(shape.result, "()");
    if (!styxcall_reply_read(STYX_MANAGED_REPLY_TAG, reply, reply_size, fields) ||
        !(empty ? styxmanaged_is_nil(fields[0].bytes, fields[0].size) :
            styxmanaged_list_read(STYX_DYNAMIC_REPLY_TAG, shape.result, fields[0].bytes,
                fields[0].size, &decoded.result)) ||
        !updates_valid(shape.arguments, &arguments, fields[1].bytes, fields[1].size,
            &decoded.changed, &decoded.updates))
    {
        return STYXRPC_INVALID;
    }
    decoded.void_result = empty;
    *result = decoded;
    return STYXRPC_OK;
}
