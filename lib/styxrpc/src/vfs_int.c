/* Copyright (c) 2026 bhgv
 * EN: Bind an Int32 variable to its exact opened fid and transfer one complete value.
 * RU: Привязать Int32 к точному открытому fid и передать одно полное значение.
 */
#include "vfs_client_internal.h"
#include "styxrpc/variable_file.h"


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open_identified_until(StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity, StyxRpcVfsStop stop, void *state)
{
    if (!out)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *fid = NULL;
    StyxRpcIoOutcome status = styxrpc_vfs_open_variable_fid(session, path, writable, &wait, &fid);
    if (status != STYXRPC_IO_OK)
    {
        return status == STYXRPC_IO_CANCELLED ? status : STYXRPC_IO_NOT_OPEN;
    }
    StyxMeta info;
    status = styxrpc_vfs_control_stat(fid, &info, &wait, STYXRPC_IO_FAILED);
    if (status == STYXRPC_IO_OK && (info.directory || info.length != STYX_VARIABLE_INT_SIZE))
    {
        status = STYXRPC_IO_FAILED;
    }
    if (status != STYXRPC_IO_OK)
    {
        styxrpc_vfs_control_close(fid, &wait);
        return status;
    }
    if (identity)
    {
        *identity = info.identity;
    }
    *out = fid;
    return STYXRPC_IO_OK;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open_identified(StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity)
{
    return styxrpc_vfs_int_open_identified_until(session, path, writable, out, identity, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_open(StyxClient *session, const char *path, int writable, StyxRpcVfsFile **out)
{
    return styxrpc_vfs_int_open_identified(session, path, writable, out, NULL);
}


/* EN: Store the decoded number in the borrowed int at state. Always accepts it.
 * RU: Записать число в заимствованный int по state. Всегда принимает значение. */
static int store_int(void *state, int32_t value)
{
    *(int32_t *)state = value;
    return 1;
}


/* EN: Copy the immutable int at state into value without side effects.
 * RU: Скопировать постоянный int из state в value без побочных эффектов. */
static int load_int(void *state, int32_t *value)
{
    *value = *(const int32_t *)state;
    return 1;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_read_until(StyxRpcVfsFile *file, int32_t *value, StyxRpcVfsStop stop, void *state)
{
    if (!file || !value)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    StyxTransfer part;
    styxclientreaduntil(file, 0, bytes, sizeof(bytes), stop, state, &part);
    if (stop && part.cancelled && !part.success)
    {
        return STYXRPC_IO_CANCELLED;
    }
    StyxRpcIoOutcome status = styxrpc_vfs_transfer_outcome(&part, sizeof(bytes));
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    const StyxIntVariable copy = {value, NULL, store_int};
    return styxintvariablewrite(&copy, 0, bytes, part.count) ? STYXRPC_IO_OK : STYXRPC_IO_FAILED;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value)
{
    return styxrpc_vfs_int_read_until(file, value, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_write_until(StyxRpcVfsFile *file, int32_t value, StyxRpcVfsStop stop, void *state)
{
    if (!file)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    const StyxIntVariable copy = {&value, load_int, NULL};
    unsigned char bytes[STYX_VARIABLE_INT_SIZE];
    size_t size = sizeof(bytes);
    if (!styxintvariableread(&copy, 0, bytes, &size) || size != sizeof(bytes))
    {
        return STYXRPC_IO_FAILED;
    }
    StyxTransfer part;
    styxclientwriteuntil(file, 0, bytes, size, stop, state, &part);
    if (stop && part.cancelled && !part.success && !part.uncertain)
    {
        return STYXRPC_IO_CANCELLED;
    }
    StyxRpcIoOutcome status = styxrpc_vfs_transfer_outcome(&part, size);
    return status == STYXRPC_IO_OK && part.count != size ? STYXRPC_IO_FAILED : status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value)
{
    return styxrpc_vfs_int_write_until(file, value, NULL, NULL);
}
