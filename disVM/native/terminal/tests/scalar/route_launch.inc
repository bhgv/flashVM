/* Copyright (c) 2026 bhgv
 * EN: Exercise one-shot launch traffic beside mounted functions on a single session.
 * RU: Проверить одноразовый запуск рядом с вызовами функций на одной сессии.
 */

/* EN: Submit one launch record and read completion on the existing library session.
 * RU: Отправить запись запуска и прочитать результат на той же сессии библиотек. */
static void check_shared_launch(StyxUnibusServer *server, StyxClient *client)
{
    StyxFid *input = styxclientwalk(client, ".unibus");
    StyxFid *exit_status = styxclientwalk(client, ".exit");
    StyxFid *image = styxclientwalk(client, "implementation.dis");
    StyxFid *declaration = styxclientwalk(client, ".interface");
    check(input && exit_status && image && declaration && styxclientopen(input, OWRITE) &&
        styxclientopen(exit_status, OREAD) && styxclientopen(image, OREAD) &&
        styxclientopen(declaration, OREAD), "open launch nodes beside library");
    styxclientclunk(image);
    styxclientclunk(declaration);
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY], received[sizeof(bytes)];
    size_t size;
    check(styxscalarwrite(REQUEST_TAG, STYX_SCALAR_CALL, 73, bytes, sizeof(bytes), &size),
        "format tagged launch inbox fixture");
    StyxTransfer transfer;
    styxclientwrite(input, 0, bytes, size, &transfer);
    check(transfer.success && transfer.count == size, "deliver launch on library session");
    DVM_VfsHandle reader = {0};
    check(dvm_provider_open(styxunibusserverinbox(server), ".", "r", &reader) == FVM_OK,
        "open the launch inbox on the provider");
    DVM_IoResult result = dvm_vfs_read_chunk(&reader, received, sizeof(received));
    check(result.status == FVM_OK && result.count == size && !memcmp(bytes, received, size),
        "launch record reached the dedicated inbox");
    dvm_vfs_close(&reader);
    styxclientclunk(input);
    styxclientread(exit_status, 0, received, sizeof(received), &transfer);
    check(transfer.success && transfer.count == 0, "launch status pending");
    styxunibusserverlaunchstatus(server, 0);
    styxclientread(exit_status, 0, received, sizeof(received), &transfer);
    check(transfer.success && transfer.count == 3 && !memcmp(received, "ok\n", 3),
        "read launch result on the library session");
    styxclientclunk(exit_status);
}
