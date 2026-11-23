/* Copyright (c) 2026 bhgv
 * EN: Cross-VM acceptance entry points for native hybrid table graphs.
 * RU: Точки сквозной приёмки графов гибридных таблиц.
 */
#ifndef FVM_STYXRPC_HYBRID_TEST_H
#define FVM_STYXRPC_HYBRID_TEST_H
#include "flashvm/styxrpc_graph.h"


/* EN: Serve an AVM1 module backed by local table methods on an owned fd.
 * RU: Обслужить модуль AVM1 с локальными методами таблиц на выданном fd. */
int test_hybrid_serve(FVM_Context *context, int fd);


/* EN: Exercise graph copy-back and failure from the FlashVM owner.
 * RU: Проверить графовую обратную запись и отказы на владельце FlashVM. */
int test_hybrid_call(FVM_Context *context, StyxRpcManagedTarget *target);
#endif
