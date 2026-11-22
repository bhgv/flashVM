/* Copyright (c) 2026 bhgv
 * EN: Per-fid managed call validation and staged request transport.
 * RU: Проверка управляемого вызова и сборка запроса на одном fid.
 */


/* EN: Validate one result, linking reference indices to its original request.
 * RU: Проверить результат по исходному запросу и его индексам ссылок. */
static int managed_result_valid(const StyxCallShape *shape, const StyxReplyField *field,
    const FVM_SRecord *arguments)
{
    FVM_SRecord value;
    if (!strcmp(shape->result, "()"))
    {
        return styxmanaged_is_nil(field->bytes, field->size);
    }
    if (strchr(shape->result, 'r'))
    {
        return styxmanaged_result_read(shape->result, field->bytes, field->size, shape->arguments, arguments, &value);
    }
    return styxmanaged_list_read(STYX_DYNAMIC_REPLY_TAG, shape->result, field->bytes, field->size, &value);
}


/* EN: Validate managed input and both reply positions before making the fid readable.
 * RU: Проверить управляемый запрос и обе части ответа до публикации чтения fid. */
static char *managed_call(FunctionFid *fid, const void *bytes, size_t size, uint32_t pid)
{
    const StyxScalarFunction *fn = &fid->function;
    FVM_SRecord arguments;
    if (fn->program_root)
    {
        FVM_SRecord outer, first;
        size_t used, prefix;
        StyxScalarKind kind;
        int64_t caller;
        if (size > STYX_MANAGED_LIST_LIMIT ||
            fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &outer, &used) != FVM_S_OK ||
            used != size || !styxmanaged_request_tag(outer.tag) ||
            fvm_srecord_read(outer.payload, outer.size, STYX_MANAGED_LIST_LIMIT, &first, &prefix) != FVM_S_OK ||
            !styxscalarread(STYX_CALL_BIG_TAG, outer.payload, prefix, &kind, &caller) ||
            kind != STYX_SCALAR_VALUE || caller <= 0 || caller > UINT32_MAX ||
            (fid->context_pid && fid->context_pid != (uint32_t)caller))
        {
            return Ebadarg;
        }
        pid = (uint32_t)caller;
        arguments = outer;
        arguments.payload += prefix;
        arguments.size -= prefix;
        if (!styxmanaged_request_payload(fn->dynamic_shape->arguments, &arguments) ||
            (styxmanaged_graph_required(fn->dynamic_shape->arguments) &&
                arguments.tag != STYX_MANAGED_GRAPH_TAG))
        {
            return Ebadarg;
        }
    }
    else if (!styxmanaged_list_read(fn->request_tag, fn->dynamic_shape->arguments, bytes, size, &arguments))
    {
        return Ebadarg;
    }
    DVM_Provider *self = pid ? capture_self(fn->program_root, pid, fid->cancel) : NULL;
    if (pid && !self)
    {
        return Eperm;
    }
    fid->submitted = 1;
    size_t length = 0;
    int outcome = fn->managed_method(fn->state, self, &arguments, fid->reply, fid->reply_capacity, &length);
    dvm_provider_release(self);
    if (outcome < 0 && fid->report_transport_failure)
    {
        return Ehungup;
    }
    if (outcome != 1)
    {
        return styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, fid->reply,
            fid->reply_capacity, &fid->size) ? nil : Ebadarg;
    }
    StyxReplyField fields[2];
    FVM_SRecord updates;
    char selected[STYX_SHAPE_MAX + 1];
    unsigned mask;
    StyxFieldDelta deltas[STYX_CALL_VALUES];
    size_t delta_count;
    unsigned char expanded[STYX_MANAGED_LIST_LIMIT];
    if (length > fid->reply_capacity || !styxcall_reply_read(fn->reply_tag, fid->reply, length, fields) ||
        (arguments.tag == STYX_MANAGED_GRAPH_TAG ?
            !styxmanaged_graph_reply(fn->dynamic_shape, &arguments, fields) :
        (!managed_result_valid(fn->dynamic_shape, &fields[0], &arguments) ||
        (styxmanaged_has_snapshot(arguments.tag) ?
            !styxmanaged_deltas_base(fn->dynamic_shape->arguments, fields[1].bytes, fields[1].size,
                deltas, &delta_count, &arguments, expanded, sizeof(expanded)) :
         arguments.tag == STYX_MANAGED_DELTA_REQUEST_TAG ?
            !styxmanaged_deltas(fn->dynamic_shape->arguments, fields[1].bytes, fields[1].size, deltas, &delta_count) :
            !styxmanaged_updates(fn->dynamic_shape->arguments, fields[1].bytes, fields[1].size,
                selected, &mask, &updates)))))
    {
        return Ebadarg;
    }
    fid->size = length;
    return nil;
}


