/* Copyright (c) 2026 bhgv
 * EN: Render Limbo declarations from the same resolved types registered for RPC.
 * RU: Вывод объявлений Limbo из разрешённых типов, зарегистрированных для RPC.
 */
#ifndef STYXRPC_INTERFACE_H
#define STYXRPC_INTERFACE_H
#include "styxrpc/managed.h"

#ifdef __cplusplus
extern "C" {
#endif


/* EN: Name the fields omitted by the transport schema but used by the Limbo signer.
 * RU: Задать имена полей, которых нет в сетевой схеме, но которые учитывает Limbo. */
typedef struct StyxRpcAdt
{
    const char *shape;         /* Exact reference token, such as r[i]. */
    const char *name;          /* Limbo ADT name. */
    const char *const *fields; /* One field name per immediate value inside brackets. */
    size_t count;             /* Number of field names. */
} StyxRpcAdt;


/* EN: One Limbo function declaration using the exact registered resolved schema.
 * RU: Объявление функции Limbo с точной схемой зарегистрированного экспорта. */
typedef struct StyxRpcDeclaration
{
    const char *name;          /* Published function name. */
    const StyxRpcSchema *schema; /* Borrowed resolved schema. */
} StyxRpcDeclaration;


/* EN: Produce a NUL-terminated .m source into a disjoint caller buffer.
 * used excludes NUL. If bytes is NULL and capacity zero, report required size.
 * One ADT per reference token. Compile this source with Limbo to get signatures.
 * RU: Сформировать .m в отдельном буфере приложения с завершающим NUL.
 * used не включает NUL. При NULL bytes и нулевой ёмкости выдать нужный размер.
 * Один ADT на токен ref. Сигнатуры из этого источника вычисляет Limbo. */
StyxRpcStatus styxrpc_module_source(const char *module, const StyxRpcDeclaration *functions, size_t count,
    const StyxRpcAdt *adts, size_t adt_count, char *bytes, size_t capacity, size_t *used);

#ifdef __cplusplus
}
#endif
#endif
