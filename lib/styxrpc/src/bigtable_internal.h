/* Copyright (c) 2026 bhgv
 * EN: Bind one registered hybrid table node to its command-file callbacks.
 * RU: Привязать узел гибридной таблицы к колбэкам командного файла.
 */
#ifndef STYXRPC_BIGTABLE_INTERNAL_H
#define STYXRPC_BIGTABLE_INTERNAL_H
#include "styxrpc/bigtable.h"

typedef enum BigNode
{
    BigIndex,  /* .idx */
    BigValue,  /* .val */
    BigLength, /* .len */
    BigCount,  /* .count */
    BigType,   /* .type */
    BigInsert, /* .ins */
    BigPush,   /* .push */
    BigPop,    /* .pop */
    BigAdd     /* .add */
} BigNode;


/* EN: Attach existing stable slots for a table directory and this child operation.
 * RU: Закрепить устойчивые слоты каталога таблицы и дочерней операции. */
void styxrpc_bigtable_bind(StyxRpcRegistration *entry, StyxRpcRegistration *directory, BigNode operation);
#endif
