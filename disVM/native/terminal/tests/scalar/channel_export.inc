/* Copyright (c) 2026 bhgv
 * EN: Verify isolation and revocation of native forward channel leases.
 * RU: Проверка изоляции и отзыва нативных аренд прямых каналов.
 */


/* EN: Echo the stored byte only after an authorized operation wrote it.
 * RU: Вернуть сохранённый байт только после записи допущенной операцией. */
static void read_lease_byte(void *state, void *bytes, size_t size, DVM_IoResult *out)
{
    unsigned char *stored = state;
    *out = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX, .count = size ? 1 : 0};
    if (size)
    {
        *(unsigned char *)bytes = *stored;
    }
}


/* EN: Store exactly one byte in the borrowed test provider.
 * RU: Сохранить ровно один байт в заимствованном провайдере теста. */
static void write_lease_byte(void *state, const void *bytes, size_t size, DVM_IoResult *out)
{
    *out = (DVM_IoResult){.status = FVM_ERROR, .stop = DVM_IO_DENIED};
    if (size == 1)
    {
        *(unsigned char *)state = *(const unsigned char *)bytes;
        *out = (DVM_IoResult){.status = FVM_OK, .flags = DVM_IO_PREFIX, .count = 1};
    }
}


/* EN: The test handle only borrows storage and has no work on close.
 * RU: Handle теста только заимствует память, освобождать нечего. */
static void release_lease_byte(void *state)
{
    (void)state;
}


/* EN: Publish one borrowed byte through the test provider's read-write file.
 * RU: Открыть один заимствованный байт файлом провайдера для чтения и записи. */
static FVM_Result open_lease_byte(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    static const DVM_VfsOps operations = {.read_chunk = read_lease_byte,
        .write_chunk = write_lease_byte, .close = release_lease_byte}; /* Test-only byte transfer. */
    if (strcmp(path, ".") || strcmp(mode, "rw"))
    {
        return FVM_ERROR;
    }
    *out = (DVM_VfsHandle){.ops = &operations, .state = state};
    return FVM_OK;
}


/* EN: Only the test file exists; fill its empty metadata on success.
 * RU: Существует только тестовый файл. При успехе заполнить метаданные. */
static FVM_Result info_lease_byte(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    if (strcmp(path, "."))
    {
        return FVM_NOT_FOUND;
    }
    *out = (FVM_FileInfo){.mode = 0666};
    return FVM_OK;
}


/* EN: Require an exact published lease in the selected module and honor clunk.
 * RU: Потребовать точную опубликованную аренду выбранного модуля и учесть clunk. */
static int exported_channel_leases(void)
{
    static const DVM_ProviderOps operations = {.info = info_lease_byte,
        .open = open_lease_byte, .destroy = release_lease_byte}; /* Test-only native handler. */
    unsigned char byte = 0x6b;
    StyxChannelExports *module = styxchannel_exports_create();
    StyxChannelExports *foreign = styxchannel_exports_create();
    DVM_Provider *control = styxchannel_export_create(module);
    DVM_Provider *bus = dvm_provider_create_native(&operations, &byte, NULL, 1, 0);
    DVM_Cancel *cancel = styxchannel_export_cancel(control);
    DVM_VfsHandle first = {0}, second = {0}, invalid = {0};
    unsigned char token[STYX_CHANNEL_EXPORT_TOKEN_SIZE], result = 0;
    int valid = module && foreign && cancel && bus && styxchannel_export_token(control, token) &&
        styxchannel_export_publish(control, bus) &&
        styxchannel_export_open(module, &first) && styxchannel_export_open(foreign, &second) &&
        styxchannel_export_open(module, &invalid);
    if (valid)
    {
        unsigned char wrong[STYX_CHANNEL_EXPORT_TOKEN_SIZE];
        memcpy(wrong, token, sizeof(wrong));
        wrong[0] ^= 0x80;
        unsigned char data = 0x37;
        valid = dvm_vfs_write_chunk(&second, token, sizeof(token)).status != FVM_OK &&
            dvm_vfs_write_chunk(&invalid, wrong, sizeof(wrong)).status != FVM_OK &&
            dvm_vfs_write_chunk(&first, token, 1).count == 1 &&
            dvm_vfs_write_chunk(&first, token + 1, sizeof(token) - 1).count == sizeof(token) - 1 &&
            dvm_vfs_write_chunk(&first, &data, 1).count == 1 &&
            dvm_vfs_read_chunk(&first, &result, 1).count == 1 && result == data;
    }
    styxchannel_export_revoke(control);
    valid = valid && dvm_cancel_requested(cancel) &&
        dvm_vfs_read_chunk(&first, &result, 1).status != FVM_OK &&
        !styxchannel_export_token(control, token);
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    dvm_vfs_close(&invalid);
    dvm_cancel_release(cancel);
    dvm_provider_release(bus);
    dvm_provider_release(control);
    styxchannel_exports_destroy(foreign);
    styxchannel_exports_destroy(module);
    return valid;
}
