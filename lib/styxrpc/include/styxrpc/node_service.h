/* Copyright (c) 2026 bhgv
 * EN: Portable execute-fid codec for hosts providing their own node registry.
 * RU: Переносимый кодек fid исполнения для хостов со своим реестром узлов.
 */
#ifndef STYXRPC_NODE_SERVICE_H
#define STYXRPC_NODE_SERVICE_H
#include "styxrpc/nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcNodeService StyxRpcNodeService; /* One exact execute fid. */
typedef int (*StyxRpcNodeInvoke)(void *context, const char *path,
    void *bytes, size_t size, size_t *used); /* Borrowed path and mutable bytes until return. */


/* EN: Create a private request cursor. Borrow context until close.
 * RU: Создать отдельный курсор запроса. Заимствовать context до закрытия. */
StyxRpcNodeService *styxrpc_node_service_open(StyxRpcNodeInvoke invoke, void *context);


/* EN: Accept a sequential fragment. The final fragment invokes the callback
 * once. Return 1 and the confirmed count on success, 0 on refusal.
 * RU: Принять последовательный фрагмент. Последний один раз вызывает
 * колбэк. Вернуть 1 и принятый размер при успехе, 0 при отказе. */
int styxrpc_node_service_write(StyxRpcNodeService *file, uint64_t offset,
    const void *input, size_t size, size_t *used);


/* EN: Read a positional slice of the successful result. Return 0 before completion.
 * RU: Прочитать фрагмент готового ответа со смещения. До завершения вернуть 0. */
int styxrpc_node_service_read(StyxRpcNodeService *file, uint64_t offset,
    void *bytes, size_t size, size_t *used);


/* EN: Consume the cursor outside I/O. An incomplete request has no effect.
 * RU: Освободить курсор вне операций I/O. Неполный запрос не выполняется. */
void styxrpc_node_service_close(StyxRpcNodeService *file);

#ifdef __cplusplus
}
#endif
#endif
