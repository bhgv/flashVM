/* Copyright (c) 2026 bhgv
 * EN: Exercise a large typed request and two-position reply on one real Styx fid.
 * RU: Проверить большой типизированный запрос и двухпозиционный ответ на Styx fid.
 */


/* EN: Echo the complete string and return an empty list of updates.
 * RU: Вернуть целую строку и пустой список обновлений. */
static int echo_managed(void *state, DVM_Provider *context, const FVM_SRecord *arguments,
    unsigned char *reply, size_t capacity, size_t *used)
{
    (void)state;
    (void)context;
    unsigned char result[240], updates[16];
    size_t a, b;
    return styxmanaged_list_write(STYX_DYNAMIC_REPLY_TAG, "(s)", arguments->payload, arguments->size,
            result, sizeof(result), &a) &&
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, NULL, 0, updates, sizeof(updates), &b) == FVM_S_OK &&
        styxcall_reply_write(STYX_MANAGED_REPLY_TAG, result, a, updates, b, reply, capacity, used);
}


/* EN: Stage a managed value exceeding the scalar profile and verify its reply.
 * RU: Собрать управляемое значение длиннее скалярного лимита и проверить ответ. */
static void managed_fid(void)
{
    int pair[2];
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "managed function carrier");
    int breaker = dup(pair[0]);
    check(breaker >= 0, "managed shutdown descriptor");
    const StyxCallShape shape = {"(s)", "(s)"};
    StyxScalarFunction function = {.request_tag = STYX_MANAGED_REQUEST_TAG, .reply_tag = STYX_MANAGED_REPLY_TAG,
        .dynamic_shape = &shape, .managed_method = echo_managed};
    StyxUnibusServer *server = styxunibusserverstartfunction(
        phyposixfd(pair[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), "managed.dis", "echo", &function);
    check(server != NULL, "export managed function");
    StyxClient *client = styxclientmountphy(phyposixfd(pair[0], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        256, "caller", "", 3000, 4);
    check(client != NULL, "mount managed function server");
    StyxFid *fid = styxclientwalk(client, "mounted/managed.dis/echo");
    check(fid && styxclientopen(fid, ORDWR), "open managed fid");
    unsigned char text[176], payload[200], request[220], wire[240];
    memset(text, 'x', sizeof(text));
    text[42] = 0;
    size_t payload_size, size, used;
    check(fvm_srecord_write(STYX_MANAGED_STRING_TAG, text, sizeof(text), payload, sizeof(payload), &payload_size)
            == FVM_S_OK &&
        styxmanaged_list_write(STYX_MANAGED_REQUEST_TAG, "(s)", payload, payload_size, request, sizeof(request), &size),
        "encode managed request");
    size_t offset = stage_request(fid, request, size / 2);
    check(offset > 0 && fvm_srecord_write(STYX_CALL_CHUNK_TAG, request + size / 2, size - size / 2,
        wire, sizeof(wire), &used) == FVM_S_OK, "stage first managed chunk");
    StyxTransfer transfer;
    styxclientwrite(fid, offset, wire, used, &transfer);
    check(transfer.success && transfer.count == used, "stage second managed chunk");
    offset += used;
    check(fvm_srecord_write(STYX_CALL_END_TAG, NULL, 0, wire, sizeof(wire), &used) == FVM_S_OK, "commit managed request");
    styxclientwrite(fid, offset, wire, used, &transfer);
    check(transfer.success && transfer.count == used, "dispatch assembled managed request");
    styxclientread(fid, 0, wire, sizeof(wire), &transfer);
    StyxReplyField parts[2];
    FVM_SRecord result;
    check(transfer.success && styxcall_reply_read(STYX_MANAGED_REPLY_TAG, wire, transfer.count, parts) &&
        styxmanaged_list_read(STYX_DYNAMIC_REPLY_TAG, "(s)", parts[0].bytes, parts[0].size, &result) &&
        result.size == payload_size && !memcmp(result.payload, payload, payload_size), "read exact managed reply");
    styxclientclunk(fid);
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxclientfree(client);
    check(styxunibusserverjoin(server), "join managed function server");
}
