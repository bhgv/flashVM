/* FlashVM: live function RPC on the resource broker's existing listener.
 * Included inside capability_c.cpp's private namespace after transport helpers. */

/* Encode one portable diagnostic integer without narrowing a native unsigned long. */
void function_fault_integer(unsigned char *data, unsigned long value)
{
#if ULONG_MAX > 0xffffffffUL
    put(data, value >> 32);
#else
    put(data, 0);
#endif
    put(data + FVM_FUNCTION_RPC_WORD_BYTES, value);
}
/* Validate native integer representability before publishing decoded diagnostics. */
bool function_fault_integer(const unsigned char *data, unsigned long &value)
{
    unsigned long high = get(data), low = get(data + FVM_FUNCTION_RPC_WORD_BYTES);
#if ULONG_MAX > 0xffffffffUL
    value = (high << 32) | low;
#else
    if (high)
        return false;
    value = low;
#endif
    return true;
}
/* Encode diagnostics by fields, never by copying process-dependent structure padding. */
void function_fault_encode(unsigned char *data, const FVM_ExceptionInfo &fault)
{
    std::memcpy(data, "FVMF\0\0\0\1", FVM_FUNCTION_RPC_FAULT_HEADER);
    const unsigned long values[] = {fault.code, fault.resource, fault.thread,
        fault.origin_thread, fault.flags, fault.line, fault.offset};
    for (unsigned i = 0; i < FVM_FUNCTION_RPC_FAULT_NUMBERS; ++i)
        function_fault_integer(data + FVM_FUNCTION_RPC_FAULT_HEADER +
            i * FVM_FUNCTION_RPC_FAULT_INTEGER_BYTES, values[i]);
#define FVM_FUNCTION_FAULT_COPY(member, offset) std::memcpy(data + offset, fault.member, sizeof(fault.member))
    FVM_FUNCTION_FAULT_COPY(message, FVM_FUNCTION_RPC_FAULT_MESSAGE);
    FVM_FUNCTION_FAULT_COPY(operation, FVM_FUNCTION_RPC_FAULT_OPERATION);
    FVM_FUNCTION_FAULT_COPY(kind, FVM_FUNCTION_RPC_FAULT_KIND);
    FVM_FUNCTION_FAULT_COPY(path, FVM_FUNCTION_RPC_FAULT_PATH);
    FVM_FUNCTION_FAULT_COPY(address, FVM_FUNCTION_RPC_FAULT_ADDRESS);
    FVM_FUNCTION_FAULT_COPY(source, FVM_FUNCTION_RPC_FAULT_SOURCE);
    FVM_FUNCTION_FAULT_COPY(function, FVM_FUNCTION_RPC_FAULT_FUNCTION);
#undef FVM_FUNCTION_FAULT_COPY
}
/* Reject malformed/truncated metadata before changing the caller's pending fault. */
bool function_fault_decode(FVM_ExceptionInfo &fault, const unsigned char *data)
{
    if (std::memcmp(data, "FVMF\0\0\0\1", FVM_FUNCTION_RPC_FAULT_HEADER))
        return false;
    unsigned long values[FVM_FUNCTION_RPC_FAULT_NUMBERS];
    for (unsigned i = 0; i < FVM_FUNCTION_RPC_FAULT_NUMBERS; ++i)
        if (!function_fault_integer(data + FVM_FUNCTION_RPC_FAULT_HEADER +
            i * FVM_FUNCTION_RPC_FAULT_INTEGER_BYTES, values[i]))
            return false;
    FVM_ExceptionInfo result;
    std::memset(&result, 0, sizeof(result));
    result.code = values[0]; result.resource = values[1]; result.thread = values[2];
    result.origin_thread = values[3]; result.flags = values[4]; result.line = values[5];
    result.offset = values[6];
#define FVM_FUNCTION_FAULT_READ(member, offset) \
    if (!std::memchr(data + offset, 0, sizeof(result.member))) return false; \
    std::memcpy(result.member, data + offset, sizeof(result.member))
    FVM_FUNCTION_FAULT_READ(message, FVM_FUNCTION_RPC_FAULT_MESSAGE);
    FVM_FUNCTION_FAULT_READ(operation, FVM_FUNCTION_RPC_FAULT_OPERATION);
    FVM_FUNCTION_FAULT_READ(kind, FVM_FUNCTION_RPC_FAULT_KIND);
    FVM_FUNCTION_FAULT_READ(path, FVM_FUNCTION_RPC_FAULT_PATH);
    FVM_FUNCTION_FAULT_READ(address, FVM_FUNCTION_RPC_FAULT_ADDRESS);
    FVM_FUNCTION_FAULT_READ(source, FVM_FUNCTION_RPC_FAULT_SOURCE);
    FVM_FUNCTION_FAULT_READ(function, FVM_FUNCTION_RPC_FAULT_FUNCTION);
#undef FVM_FUNCTION_FAULT_READ
    fault = result;
    return true;
}

/* Locate owner state only while holding the authority lock. */
FunctionOwner *function_owner(FVM_ResourceBroker *b, FVM_Context *c)
{
    for (size_t i = 0; i < b->function_owners.size(); ++i)
        if (b->function_owners[i].context == c)
            return &b->function_owners[i];
    return 0;
}

/* [GC:] Keep an original heap alive until invocation, encoding and exception cleanup finish. */
struct FunctionCallScope
{
    FVM_ResourceBroker *broker; /* Authority synchronizing the active call count. */
    FVM_Context *context; /* [GC:] Original function heap, protected against detachment. */
    /* Start without a heap lease. */
    FunctionCallScope(FVM_ResourceBroker *b) : broker(b), context(0) {}
    /* [GC:] Release the heap lease and wake a host reclaiming its context. */
    ~FunctionCallScope()
    {
        if (!context)
            return;
        fvm_exception_clear(context);
        Lock guard(broker->sync);
        FunctionOwner *owner = function_owner(broker, context);
        --owner->active;
        fvm_port_sync_signal_all(broker->sync);
    }
};

/* Invoke one original function without changing its closure, code, userdata or capture. */
FVM_Result dispatch_function(FVM_ResourceBroker *b, const unsigned char *h,
    const unsigned char *input, size_t size, unsigned char **output, size_t *length,
    FVM_ExceptionInfo &fault, bool &has_fault)
{
    FunctionCallScope call(b);
    FVM_Object *function = 0;
    {
        Lock guard(b->sync);
        if (b->stopped || std::memcmp(h + FVM_FUNCTION_RPC_SESSION_OFFSET, b->session.c_str(), FVM_FUNCTION_RPC_ID_BYTES))
            return FVM_NOT_FOUND;
        for (size_t i = 0; i < b->entries.size(); ++i)
        {
            Entry &entry = b->entries[i];
            if (entry.kind == FVM_FUNCTION_CAPABILITY_KIND &&
                !std::memcmp(h + FVM_FUNCTION_RPC_TARGET_OFFSET, entry.token.c_str(), FVM_FUNCTION_RPC_ID_BYTES))
            {
                FunctionOwner *owner = function_owner(b, entry.owner);
                if (!owner || !owner->serving || owner->active == ULONG_MAX)
                    return FVM_NOT_FOUND;
                ++owner->active;
                call.context = entry.owner;
                function = (FVM_Object *)entry.handle;
                break;
            }
        }
    }
    if (!function)
        return FVM_NOT_FOUND;
    FVM_Context *c = call.context;
    fvm_exception_clear(c);
    FVM_Value frame, result;
    FVM_Result status = fvm_value_deserialize(c, input, size, &frame);
    if (status != FVM_OK)
        return status;
    if (frame.type != FVM_VALUE_OBJECT || frame.object->indexed.empty() ||
        frame.object->indexed.size() > FVM_FUNCTION_RPC_MAX_ARGS + FVM_FUNCTION_RPC_ARGUMENT_BASE)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Receiver and copied arguments stay reachable during script collection. */
    FVM_ObjectRootScope roots(c, frame.object);
    fvm_detail::map<unsigned long, FVM_Value>::iterator first = frame.object->indexed.find(FVM_FUNCTION_RPC_RECEIVER_INDEX);
    if (first == frame.object->indexed.end() ||
        (first->second.type != FVM_VALUE_OBJECT && first->second.type != FVM_VALUE_UNDEFINED))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *receiver = first->second.type == FVM_VALUE_OBJECT ? first->second.object : 0;
    fvm_detail::vector<FVM_Value> args;
    for (size_t i = FVM_FUNCTION_RPC_ARGUMENT_BASE; i < frame.object->indexed.size(); ++i)
    {
        fvm_detail::map<unsigned long, FVM_Value>::iterator arg = frame.object->indexed.find((unsigned long)i);
        if (arg == frame.object->indexed.end())
            return FVM_INVALID_ARGUMENT;
        args.push_back(arg->second);
    }
    status = fvm_function_invoke(c, function, receiver, (unsigned int)args.size(),
                                 args.empty() ? 0 : args.begin(), &result);
    if (c->fault_pending)
    {
        fault = c->fault;
        has_fault = true;
    }
    if (status == FVM_EXCEPTION)
        result = c->exception_value;
    else if (status != FVM_OK)
        fvm_value_undefined(&result);
    FVM_Result encoded = fvm_value_serialize(c, &result, output, length);
    return encoded == FVM_OK ? status : encoded;
}

/* Echo all routing/correlation fields so a reply cannot be delivered to another call. */
void serve_function(FVM_ResourceBroker *b, FVM_PortSocket *socket, const unsigned char *prefix)
{
    unsigned char h[FVM_FUNCTION_RPC_REQUEST_SIZE];
    std::memcpy(h, prefix, FVM_FUNCTION_RPC_CALL_ID);
    if (!transfer(socket, h + FVM_FUNCTION_RPC_CALL_ID, FVM_FUNCTION_RPC_ID_BYTES, false))
        return;
    unsigned char *input = 0, *output = 0;
    size_t size = (size_t)get(h + FVM_FUNCTION_RPC_REQUEST_LENGTH), length = 0;
    FVM_Result status = FVM_INVALID_ARGUMENT;
    FVM_ExceptionInfo fault;
    bool has_fault = false;
    if (h[FVM_FUNCTION_RPC_OPERATION_OFFSET] == FVM_FUNCTION_RPC_CALL && size <= limit)
    {
        input = size ? (unsigned char *)std::malloc(size) : 0;
        if (size && !input)
            status = FVM_OUT_OF_MEMORY;
        else if (!size || transfer(socket, input, size, false))
        {
            try { status = dispatch_function(b, h, input, size, &output, &length, fault, has_fault); }
            catch (...) { status = FVM_OUT_OF_MEMORY; }
        }
    }
    unsigned char response[FVM_FUNCTION_RPC_RESPONSE_SIZE];
    std::memset(response, 0, sizeof(response));
    std::memcpy(response, "FVMR", FVM_FUNCTION_RPC_MAGIC_BYTES);
    response[FVM_FUNCTION_RPC_VERSION_OFFSET] = FVM_FUNCTION_RPC_VERSION;
    response[FVM_FUNCTION_RPC_OPERATION_OFFSET] = h[FVM_FUNCTION_RPC_OPERATION_OFFSET];
    response[FVM_FUNCTION_RPC_KIND_OFFSET] = FVM_FUNCTION_CAPABILITY_KIND;
    response[FVM_FUNCTION_RPC_FLAGS_OFFSET] = has_fault ? FVM_FUNCTION_RPC_FAULT_FLAG : 0;
    std::memcpy(response + FVM_FUNCTION_RPC_REPLY_TARGET, h + FVM_FUNCTION_RPC_TARGET_OFFSET, FVM_FUNCTION_RPC_ID_BYTES);
    std::memcpy(response + FVM_FUNCTION_RPC_REPLY_SOURCE, h + FVM_FUNCTION_RPC_SOURCE_OFFSET, FVM_FUNCTION_RPC_ID_BYTES);
    std::memcpy(response + FVM_FUNCTION_RPC_REPLY_CONTEXT, h + FVM_FUNCTION_RPC_CONTEXT_OFFSET, FVM_FUNCTION_RPC_WORD_BYTES);
    std::memcpy(response + FVM_FUNCTION_RPC_REPLY_CALL, h + FVM_FUNCTION_RPC_CALL_ID, FVM_FUNCTION_RPC_ID_BYTES);
    put(response + FVM_FUNCTION_RPC_STATUS, (unsigned long)(-status));
    put(response + FVM_FUNCTION_RPC_LENGTH, (unsigned long)length);
    if (transfer(socket, response, sizeof(response), true))
    {
        if (has_fault)
        {
            unsigned char record[FVM_FUNCTION_RPC_FAULT_SIZE];
            function_fault_encode(record, fault);
            transfer(socket, record, sizeof(record), true);
        }
        if (length)
            transfer(socket, output, length, true);
    }
    std::free(input);
    std::free(output);
}

/* Make one bounded call; lost replies never trigger an automatic re-execution. */
FVM_Result function_request(Remote &remote, FVM_Context *c, const unsigned char *input,
    size_t size, FVM_Value *out)
{
    if (c->broker && c->broker->session == remote.session &&
        fvm_port_thread_is_current(c->broker->thread))
        return FVM_UNSUPPORTED;
    if (size > limit)
        return FVM_UNSUPPORTED;
    char source[FVM_FUNCTION_RPC_ID_STORAGE], request[FVM_FUNCTION_RPC_ID_STORAGE];
    if (!nonce(source) || !hex(request))
        return FVM_ERROR;
    unsigned char h[FVM_FUNCTION_RPC_REQUEST_SIZE], response[FVM_FUNCTION_RPC_RESPONSE_SIZE];
    std::memset(h, 0, sizeof(h));
    std::memcpy(h, "FVMC", FVM_FUNCTION_RPC_MAGIC_BYTES);
    h[FVM_FUNCTION_RPC_VERSION_OFFSET] = FVM_FUNCTION_RPC_VERSION;
    h[FVM_FUNCTION_RPC_OPERATION_OFFSET] = FVM_FUNCTION_RPC_CALL;
    h[FVM_FUNCTION_RPC_KIND_OFFSET] = FVM_FUNCTION_CAPABILITY_KIND;
    std::memcpy(h + FVM_FUNCTION_RPC_SESSION_OFFSET, remote.session.c_str(), FVM_FUNCTION_RPC_ID_BYTES);
    std::memcpy(h + FVM_FUNCTION_RPC_TARGET_OFFSET, remote.token.c_str(), FVM_FUNCTION_RPC_ID_BYTES);
    std::memcpy(h + FVM_FUNCTION_RPC_SOURCE_OFFSET, source, FVM_FUNCTION_RPC_ID_BYTES);
    put(h + FVM_FUNCTION_RPC_CONTEXT_OFFSET, c->identity);
    put(h + FVM_FUNCTION_RPC_REQUEST_LENGTH, (unsigned long)size);
    std::memcpy(h + FVM_FUNCTION_RPC_CALL_ID, request, FVM_FUNCTION_RPC_ID_BYTES);
    FVM_PortSocket *socket = fvm_port_tcp_connect(remote.host.c_str(), remote.port);
    if (!socket)
    {
        fvm_resource_fault(c, FVM_RESOURCE_UNREACHABLE, 0, "function.connect", remote.uri.c_str());
        return FVM_ERROR;
    }
    fvm_port_socket_timeout(socket, FVM_FUNCTION_RPC_TIMEOUT_MS);
    bool ok = transfer(socket, h, sizeof(h), true) && transfer(socket, (void *)input, size, true) &&
              transfer(socket, response, sizeof(response), false);
    unsigned char *data = 0;
    FVM_Result status = FVM_ERROR;
    FVM_ExceptionInfo fault;
    bool has_fault = false;
    bool allocation_failed = false;
    size_t length = 0;
    if (ok)
    {
        ok = !std::memcmp(response, "FVMR", FVM_FUNCTION_RPC_MAGIC_BYTES) && response[FVM_FUNCTION_RPC_VERSION_OFFSET] == FVM_FUNCTION_RPC_VERSION &&
             response[FVM_FUNCTION_RPC_OPERATION_OFFSET] == FVM_FUNCTION_RPC_CALL && response[FVM_FUNCTION_RPC_KIND_OFFSET] == FVM_FUNCTION_CAPABILITY_KIND &&
             response[FVM_FUNCTION_RPC_FLAGS_OFFSET] <= FVM_FUNCTION_RPC_FAULT_FLAG && !std::memcmp(response + FVM_FUNCTION_RPC_REPLY_TARGET, h + FVM_FUNCTION_RPC_TARGET_OFFSET, FVM_FUNCTION_RPC_ID_BYTES) &&
             !std::memcmp(response + FVM_FUNCTION_RPC_REPLY_SOURCE, h + FVM_FUNCTION_RPC_SOURCE_OFFSET, FVM_FUNCTION_RPC_ID_BYTES) &&
             !std::memcmp(response + FVM_FUNCTION_RPC_REPLY_CONTEXT, h + FVM_FUNCTION_RPC_CONTEXT_OFFSET, FVM_FUNCTION_RPC_WORD_BYTES) &&
             !std::memcmp(response + FVM_FUNCTION_RPC_REPLY_CALL, request, FVM_FUNCTION_RPC_ID_BYTES) &&
             get(response + FVM_FUNCTION_RPC_STATUS) <= FVM_FUNCTION_RPC_MAX_RESULT;
        length = (size_t)get(response + FVM_FUNCTION_RPC_LENGTH);
        ok = ok && length <= limit;
        if (ok && response[FVM_FUNCTION_RPC_FLAGS_OFFSET])
        {
            unsigned char record[FVM_FUNCTION_RPC_FAULT_SIZE];
            std::memset(&fault, 0, sizeof(fault));
            ok = transfer(socket, record, sizeof(record), false) && function_fault_decode(fault, record);
            has_fault = ok;
        }
        if (ok && length)
        {
            data = (unsigned char *)std::malloc(length);
            allocation_failed = data == 0;
            ok = data && transfer(socket, data, length, false);
        }
        if (ok)
            status = (FVM_Result)(-(int)get(response + FVM_FUNCTION_RPC_STATUS));
    }
    fvm_port_socket_close(socket);
    if (!ok)
    {
        std::free(data);
        if (allocation_failed)
            return FVM_OUT_OF_MEMORY;
        fvm_resource_fault(c, FVM_RESOURCE_DISCONNECTED, 0, "function.reply", remote.uri.c_str());
        return FVM_ERROR;
    }
    FVM_Value result;
    fvm_value_undefined(&result);
    FVM_Result decoded = length ? fvm_value_deserialize(c, data, length, &result) : FVM_OK;
    std::free(data);
    if (decoded != FVM_OK)
        return decoded;
    FVM_Result raised = status;
    if (status == FVM_EXCEPTION)
        raised = fvm_exception_raise(c, &result);
    if (has_fault && (status != FVM_EXCEPTION || raised == FVM_EXCEPTION))
    {
        c->fault = fault;
        c->fault.flags |= FVM_EXCEPTION_REMOTE_CONTEXT | FVM_EXCEPTION_NETWORK;
        c->fault_pending = true;
    }
    if (status == FVM_EXCEPTION)
        return raised;
    if (status == FVM_OK)
        *out = result;
    return status;
}

/* Serialize receiver/arguments into one graph, preserving aliases within this call. */
FVM_Result function_proxy(FVM_Context *c, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *argv, FVM_Value *out, void *raw)
{
    if (argc > FVM_FUNCTION_RPC_MAX_ARGS)
        return FVM_UNSUPPORTED;
    FVM_Object *capture = (FVM_Object *)raw;
    if (!capture || capture->owner != c)
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator address = capture->properties.find("uri");
    if (address == capture->properties.end() || address->second.type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    Remote remote;
    if (!parse(address->second.string, remote))
        return FVM_INVALID_ARGUMENT;
    FVM_Object *frame = 0;
    FVM_Result result = fvm_object_create(c, &frame);
    if (result != FVM_OK)
        return result;
    /* [GC:] Invocation already retains the proxy/capture; this scope retains its argument frame. */
    FVM_ObjectRootScope roots(c, frame);
    FVM_Value self;
    fvm_value_undefined(&self);
    if (receiver && receiver != c->global) { self.type = FVM_VALUE_OBJECT; self.object = receiver; }
    frame->indexed[FVM_FUNCTION_RPC_RECEIVER_INDEX] = self;
    for (unsigned int i = 0; i < argc; ++i)
        frame->indexed[(unsigned long)i + FVM_FUNCTION_RPC_ARGUMENT_BASE] = argv[i];
    unsigned char *data = 0;
    size_t size = 0;
    self.type = FVM_VALUE_OBJECT;
    self.object = frame;
    result = fvm_value_serialize(c, &self, &data, &size);
    if (result == FVM_OK)
        result = function_request(remote, c, data, size, out);
    fvm_serialized_free(data);
    return result;
}
