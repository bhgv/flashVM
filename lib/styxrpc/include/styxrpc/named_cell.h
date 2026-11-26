/* Copyright (c) 2026 bhgv
 * EN: Leased value cells carried by reference on named StyxRPC channels.
 * RU: Арендуемые ячейки значений, передаваемые по ссылке в именованных каналах.
 */
#ifndef STYXRPC_NAMED_CELL_H
#define STYXRPC_NAMED_CELL_H
#include "styxrpc/named_channel.h"

#define STYXRPC_CELL_NONCE 16U /* Entropy supplied by the issuing registry. */
#define STYXRPC_CELL_TOKEN 24U /* Nonce plus a never-reused registry serial. */
#define STYXRPC_CELL_NEW 4U /* Create and hold a cell, reply with its token. */
#define STYXRPC_CELL_ATTACH 5U /* Hold the cell identified by one token. */
#define STYXRPC_CELL_GET 6U /* Reply with the current value frame. */
#define STYXRPC_CELL_SET 7U /* Replace the value frame. */
#define STYXRPC_NAMED_CELL_REF 8U /* Channel value tag followed by one cell token. */

#ifdef __cplusplus
extern "C" {
#endif

/* EN: Check one counted tagged scalar frame for the shared-cell profile.
 * RU: Проверить скалярный кадр с тегом и длиной для общей ячейки. */
int styxrpc_cell_scalar(const void *frame, size_t size);


/* EN: Check the borrowed tagged frame and size. Scalars and checked FVMG envelopes
 * qualify. The receiving VM validates the graph body. Returns one on acceptance.
 * RU: Проверить заимствованный кадр и длину. Принимаются скаляры и проверенные
 * оболочки FVMG. Тело графа проверяет принимающая VM. Возвращает единицу при успехе. */
int styxrpc_cell_value(const void *frame, size_t size);

/* EN: Supply issuer entropy before serving endpoints. Callback fills bytes and
 * returns one. It must be thread-safe and must not reenter the registry.
 * RU: Задать источник энтропии до обслуживания участников. Колбэк заполняет
 * bytes и возвращает единицу. Он потокобезопасен, повторный вход запрещён. */
typedef int (*StyxRpcCellEntropy)(void *state, unsigned char *bytes, size_t size);


/* EN: Enable cell creation on a fresh registry. Borrow state until destruction.
 * RU: Включить создание ячеек в новом реестре. state заимствован до удаления. */
int styxrpc_named_cells_enable(StyxRpcNamedRegistry *registry, StyxRpcCellEntropy entropy, void *state);


/* EN: Create a value cell, returning an owned lease and a token.
 * RU: Создать ячейку значения и вернуть собственную аренду и жетон. */
StyxRpcVfsFile *styxrpc_cell_create(struct StyxClient *session,
    const void *frame, size_t size, void *token);

/* EN: Acquire a new owned fid for an existing token on its issuing session.
 * RU: Получить собственный fid для действующего жетона в выдавшей сессии. */
StyxRpcVfsFile *styxrpc_cell_attach(struct StyxClient *session, const void *token);

/* EN: Read one owned value frame. Release it with free. Serialize fid operations.
 * RU: Получить собственный кадр значения. Освободить через free. Операции fid последовательны. */
StyxRpcIoOutcome styxrpc_cell_get(StyxRpcVfsFile *lease, unsigned char **frame, size_t *size);

/* EN: Atomically replace the value with a borrowed value frame. Never retry an uncertain write.
 * RU: Атомарно заменить значение заимствованным кадром значения. Без повтора при неопределённости. */
StyxRpcIoOutcome styxrpc_cell_set(StyxRpcVfsFile *lease, const void *frame, size_t size);

#ifdef __cplusplus
}
#endif
#endif
