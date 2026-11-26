/* Copyright (c) 2026 bhgv
 * EN: Adapt the portable execute fid to the callback slot tree.
 * RU: Подключить переносимый fid исполнения к дереву колбэков.
 */
#include "nodes_internal.h"
#include "styxrpc/node_service.h"


/* EN: Admit one read/write fid for the slot's live registry.
 * RU: Открыть fid чтения и записи для действующего реестра слота. */
static int open_exec(void *nodes, uint64_t id, unsigned access, void **cookie)
{
    if (!nodes || id != STYXRPC_NODE_EXEC_QID || access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    *cookie = styxrpc_node_service_open(styxrpc_nodes_exec, nodes);
    return *cookie != NULL;
}


/* EN: Pass a positioned write into the shared codec.
 * RU: Передать позиционную запись общему кодеку. */
static int request(void *file, uint64_t offset, const void *input, size_t size, size_t *used)
{
    return styxrpc_node_service_write(file, offset, input, size, used);
}


/* EN: Read a confirmed slice without rerunning the callback.
 * RU: Прочитать подтверждённый фрагмент без повторного вызова колбэка. */
static int reply(void *file, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    return styxrpc_node_service_read(file, offset, bytes, size, used);
}


/* EN: Dispose the exact cursor and all its staged bytes.
 * RU: Освободить точный курсор и все сохранённые байты. */
static void close_exec(void *file)
{
    styxrpc_node_service_close(file);
}

const StyxRpcVfs styxrpc_node_exec_files = {NULL, open_exec, reply, request, close_exec}; /* Hidden node service. */
