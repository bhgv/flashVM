/* Copyright (c) 2026 bhgv
 * EN: A named-channel reference to an exact published Int32 module variable.
 * RU: Ссылка именованного канала на точную опубликованную переменную Int32.
 */
#ifndef STYXRPC_MODULE_REF_H
#define STYXRPC_MODULE_REF_H
#include "styxrpc/vfs.h"
#include "styxrpc/vfs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_MODULE_REF_SCOPE_SIZE 16U /* Nonce for one paired connection. */
#define STYXRPC_MODULE_REF_HEADER 28U /* Magic, version, flags, Qid and scope. */
#define STYXRPC_MODULE_REF_WIRE_MAX (STYXRPC_MODULE_REF_HEADER + STYXRPC_VFS_PATH_MAX) /* Maximum payload bytes. */
#define STYXRPC_MODULE_REF_FORWARD 0U /* Publication on the forward player's ordinary export. */
#define STYXRPC_MODULE_REF_REVERSE 1U /* Publication on the reverse player's ordinary export. */

struct StyxRpcModuleSlot; /* Owner of the current publication. */

typedef struct StyxRpcModuleRef
{
    uint64_t node; /* Qid.path of the opened export, including publication generation. */
    unsigned owner; /* Which peer issued the original ordinary VFS identity. */
    int writable; /* The original opening allows replacement when one. */
    char path[STYXRPC_VFS_PATH_MAX + 1U]; /* Canonical mounted/<image>/.var/<export> spelling. */
} StyxRpcModuleRef;


/* EN: Accept only the canonical hosted variable path spelling.
 * RU: Принять только канонический путь опубликованной переменной. */
int styxrpc_module_ref_path(const char *path);


/* EN: Encode the descriptor with scope into capacity bytes. Zero rejects it.
 * RU: Закодировать дескриптор со scope в capacity байтах. Ноль означает отказ. */
size_t styxrpc_module_ref_encode(const StyxRpcModuleRef *ref, const void *scope, void *bytes, size_t capacity);


/* EN: Require the expected scope and exact record, leaving out untouched on error.
 * RU: Проверить ожидаемый scope и точную запись, сохранив out при ошибке. */
int styxrpc_module_ref_decode(const void *bytes, size_t size, const void *scope, StyxRpcModuleRef *out);


/* EN: Open ref on its ordinary issuer mount. Require the held file Qid to match.
 * RU: Открыть ref в обычной сессии владельца и сверить Qid открытого файла. */
StyxRpcIoOutcome styxrpc_module_ref_open_remote(struct StyxClient *issuer,
    const StyxRpcModuleRef *ref, StyxRpcVfsFile **out);


/* EN: Open the exact live slot export on its owner. out must be zeroed.
 * RU: Открыть точный живой экспорт слота у владельца. out изначально обнулён. */
StyxRpcStatus styxrpc_module_ref_open_local(struct StyxRpcModuleSlot *slot,
    const StyxRpcModuleRef *ref, StyxRpcFile *out);


/* EN: Read or replace one signed value using a held local export file.
 * RU: Прочитать либо записать целое знаковое значение открытого локального экспорта. */
StyxRpcStatus styxrpc_module_ref_read_local(StyxRpcFile *file, int32_t *value);
StyxRpcStatus styxrpc_module_ref_write_local(StyxRpcFile *file, int32_t value);

#ifdef __cplusplus
}
#endif
#endif
