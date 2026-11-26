/* Copyright (c) 2026 bhgv
 * EN: Registry-owned cells. Call under its operation lock.
 * RU: Ячейки реестра. Вызовы идут под его блокировкой операций.
 */
#ifndef STYXRPC_NAMED_CELL_STORE_H
#define STYXRPC_NAMED_CELL_STORE_H
#include "styxrpc/named_cell.h"

typedef struct NamedCell NamedCell; /* Owned while any fid or transfer pins it. */

typedef struct NamedCells
{
    NamedCell *head; /* First live cell, or NULL. */
    uint64_t latest; /* Last serial. Exhaustion refuses further creation. */
} NamedCells;


/* EN: Allocate one leased cell copied from nonce and a validated value frame.
 * RU: Выделить арендованную ячейку с копией nonce и проверенного кадра значения. */
NamedCell *styxrpc_cells_create(NamedCells *, const unsigned char *nonce, const void *frame,
    size_t size, unsigned char *token);

/* EN: Acquire the exact live token. NULL for missing cells or overflow.
 * RU: Удержать точный живой жетон. NULL при отсутствии или переполнении. */
NamedCell *styxrpc_cells_attach(NamedCells *, const unsigned char *token);

/* EN: Release one retained cell. Last release frees its value and token.
 * RU: Освободить ссылку. Последняя освобождает значение и жетон. */
void styxrpc_cells_release(NamedCells *, NamedCell *);

/* EN: Copy the current frame to freshly allocated storage.
 * RU: Скопировать текущий кадр в новый буфер. */
unsigned char *styxrpc_cells_get(const NamedCell *, size_t *size);

/* EN: Consume the validated value frame and replace the old value.
 * RU: Принять проверенный кадр во владение и заменить старое значение. */
void styxrpc_cells_set(NamedCell *, unsigned char *frame, size_t size);
#endif
