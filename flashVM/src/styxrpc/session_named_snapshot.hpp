/* Copyright (c) 2026 bhgv
 * EN: By-value capture of a named StyxRPC graph with transparent live slots.
 * RU: Снимок графа именованного StyxRPC по значению с прозрачными живыми слотами.
 */
#ifndef FVM_NAMED_SNAPSHOT_HPP
#define FVM_NAMED_SNAPSHOT_HPP
#include "flashvm/value.h"


/* [ASINC] [GC:] EN: Capture value on the owning context thread. On success data
 * contains a graph snapshot owned by the caller. Release with fvm_serialized_free.
 * On failure data is NULL and size zero. Sampling can wait for the peer.
 * RU: Снять value в потоке контекста. При успехе data содержит снимок во владении
 * вызывающего кода, освободить через fvm_serialized_free. При ошибке data равно
 * NULL и size равно нулю. Чтение может ждать ответа участника. */
FVM_Result fvm_named_snapshot(FVM_Context *, const FVM_Value *, unsigned char **data, size_t *size);
#endif
