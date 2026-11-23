/* Copyright (c) 2026 bhgv
 * EN: Local ActionScript tables compatible with Dis Bigtable graph references.
 * RU: Локальные таблицы ActionScript, совместимые с графовыми ссылками Bigtable Dis.
 */
#ifndef FLASHVM_STYXRPC_HYBRID_H
#define FLASHVM_STYXRPC_HYBRID_H
#include "flashvm/value.h"

#ifdef __cplusplus
extern "C" {
#endif


/* [GC:] EN: Register a no-argument constructor under the supplied AS global name.
 * RU: Зарегистрировать конструктор без аргументов под указанным глобальным именем AS. */
FVM_API FVM_Result fvm_styxrpc_hybrid_bind(FVM_Context *context, const char *global);

#ifdef __cplusplus
}
#endif
#endif
