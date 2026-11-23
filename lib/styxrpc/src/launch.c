/* Copyright (c) 2026 bhgv
 * EN: Existing one-shot launch profile, definite outcomes, and no replay.
 * RU: Существующий профиль запуска, подтверждённый исход и отсутствие повторов.
 */
#include "styxrpc/launch.h"
#include "client.h"
#include "flashvm/srecord.h"
#include <string.h>
#include <stdio.h>

#define LAUNCH_TAG 17UL     /* Existing DisVM launch request tag. */
#define LAUNCH_LIMIT 65536U /* Maximum startup argument extent. */
#define NAMED_LIMIT 64U     /* Complete record limit of the persistent provider. */
#define PROGRAM_HEADER 13U  /* Version and three network-order u32 fields. */
#define PROGRAM_NAME 31U    /* Maximum basename accepted by the provider. */


/* EN: Gather a complete short file or pending empty status on one opened fid.
 * RU: Собрать короткий файл или пустой статус целиком на одном открытом fid. */
static int read_exact(StyxFid *fid, char *bytes, size_t capacity, size_t *size)
{
    *size = 0;
    while (fid && *size < capacity)
    {
        StyxTransfer got = {0};
        styxclientread(fid, *size, bytes + *size, capacity - *size, &got);
        if (!got.success || !got.confirmed || got.uncertain)
        {
            return 0;
        }
        if (!got.count)
        {
            return 1;
        }
        *size += got.count;
    }
    return 0;
}


/* EN: Borrow client to read a bounded file from a fresh read-only fid.
 * RU: Использовать client для ограниченного чтения файла на новом fid. */
static int read_snapshot(StyxClient *client, const char *path, char *bytes, size_t capacity, size_t *size)
{
    StyxFid *fid = client ? styxclientwalk(client, path) : NULL;
    int read = fid && styxclientopen(fid, 0) && read_exact(fid, bytes, capacity, size);
    styxclientclunk(fid);
    return read;
}


/* EN: Write exactly once on an opened fid and classify its acknowledged prefix.
 * RU: Записать ровно один раз на открытый fid и проверить подтверждённый префикс. */
static StyxRpcLaunchStatus submit(StyxFid *fid, const void *bytes, size_t size)
{
    StyxTransfer sent = {0};
    styxclientwrite(fid, 0, bytes, size, &sent);
    if (sent.success && sent.confirmed && !sent.uncertain && sent.count == size)
    {
        return STYXRPC_LAUNCH_PENDING;
    }
    return sent.uncertain || sent.count || !sent.confirmed ?
        STYXRPC_LAUNCH_UNKNOWN : STYXRPC_LAUNCH_NOT_SENT;
}


/* EN: Confirm the known profile at the mounted provider root.
 * RU: Подтвердить известный профиль в корне смонтированного провайдера. */
static int has_profile(StyxClient *client, const char *profile)
{
    char offered[sizeof("disvm.launch/1 context\n")];
    size_t length;
    return read_snapshot(client, ".interface", offered, sizeof(offered), &length) &&
        length == strlen(profile) && !memcmp(offered, profile, length);
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_start(StyxClient *client, const void *arguments, size_t size,
    void *scratch, size_t capacity)
{
    size_t frame;
    if (!client || (size && !arguments) || size > LAUNCH_LIMIT || !scratch ||
        fvm_srecord_write(LAUNCH_TAG, arguments, size, scratch, capacity, &frame) != FVM_S_OK ||
        !has_profile(client, "disvm.launch/1 raw\n"))
    {
        return STYXRPC_LAUNCH_NOT_SENT;
    }
    StyxFid *fid = styxclientwalk(client, ".unibus");
    StyxRpcLaunchStatus status = fid && styxclientopen(fid, 1) ?
        submit(fid, scratch, frame) : STYXRPC_LAUNCH_NOT_SENT;
    styxclientclunk(fid);
    return status;
}


/* EN: Interpret the exact completion receipt, including an empty pending read.
 * RU: Разобрать точную квитанцию, включая пустое чтение при ожидании. */
static StyxRpcLaunchStatus result(const char *reply, size_t length)
{
    if (!length)
    {
        return STYXRPC_LAUNCH_PENDING;
    }
    if (length == 3 && !memcmp(reply, "ok\n", 3))
    {
        return STYXRPC_LAUNCH_OK;
    }
    return length == 6 && !memcmp(reply, "error\n", 6) ?
        STYXRPC_LAUNCH_FAILED : STYXRPC_LAUNCH_UNKNOWN;
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_status(StyxClient *client)
{
    char reply[7];
    size_t length;
    return read_snapshot(client, ".exit", reply, sizeof(reply), &length) ?
        result(reply, length) : STYXRPC_LAUNCH_UNKNOWN;
}


/* EN: Check one public executable basename before constructing its endpoint.
 * RU: Проверить одно публичное имя программы перед построением пути. */
static int program_name(const char *name)
{
    static const char initial[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t length = name ? strlen(name) : 0;
    return length > 4 && length <= PROGRAM_NAME && !strcmp(name + length - 4, ".dis") &&
        strchr(initial, name[0]) && strspn(name, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") == length;
}


/* EN: Encode the current provider's scoped selector, preserving the startup byte suffix.
 * RU: Кодировать выбор контекста провайдера, сохранив исходные стартовые байты. */
static int named_record(const StyxRpcProgram *program, const void *arguments, size_t size,
    void *scratch, size_t capacity, size_t *frame)
{
    unsigned char header[PROGRAM_HEADER] = {1};
    if (!program || !program->pid || program->pid == program->parent_pid || (size && !arguments) ||
        size > NAMED_LIMIT - sizeof(header) || !scratch)
    {
        return 0;
    }
    const uint32_t values[] = {program->pid, program->parent_pid, program->hidden_fds};
    for (unsigned i = 0; i < 3; ++i)
    {
        for (unsigned byte = 0; byte < 4; ++byte)
        {
            header[1 + 4 * i + byte] = (unsigned char)(values[i] >> (24 - 8 * byte));
        }
    }
    return fvm_srecord_write_pair(LAUNCH_TAG, header, sizeof(header), arguments, size,
        scratch, capacity, frame) == FVM_S_OK && *frame <= NAMED_LIMIT;
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_named(StyxClient *client, const char *name, const StyxRpcProgram *program,
    const void *arguments, size_t size, void *scratch, size_t capacity, StyxFid **request)
{
    if (!request || *request)
    {
        return STYXRPC_LAUNCH_NOT_SENT;
    }
    size_t frame;
    if (!client || !styxclientbidir(client) || !program_name(name) ||
        !named_record(program, arguments, size, scratch, capacity, &frame) ||
        !has_profile(client, "disvm.launch/1 context\n"))
    {
        return STYXRPC_LAUNCH_NOT_SENT;
    }
    char path[sizeof("programs/.") + PROGRAM_NAME];
    snprintf(path, sizeof(path), "programs/.%s", name);
    StyxFid *fid = styxclientwalk(client, path);
    StyxRpcLaunchStatus status = fid && styxclientopen(fid, 2) ?
        submit(fid, scratch, frame) : STYXRPC_LAUNCH_NOT_SENT;
    if (status == STYXRPC_LAUNCH_PENDING)
    {
        *request = fid;
    }
    else
    {
        styxclientclunk(fid);
    }
    return status;
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_named_status(StyxFid *request)
{
    char reply[7];
    size_t length;
    return read_exact(request, reply, sizeof(reply), &length) ?
        result(reply, length) : STYXRPC_LAUNCH_UNKNOWN;
}


/* EN/RU: See launch.h. / См. launch.h. */
void styxrpc_launch_named_close(StyxFid **request)
{
    if (request && *request)
    {
        StyxFid *fid = *request;
        *request = NULL;
        styxclientclunk(fid);
    }
}
