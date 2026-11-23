/* Copyright (c) 2026 bhgv
 * EN: Real graph acceptance entry points for the shared hosted fixture.
 * RU: Точки проверки настоящих графов для общего хостового теста.
 */
#ifndef FVM_STYXRPC_GRAPH_TEST_H
#define FVM_STYXRPC_GRAPH_TEST_H
#include "flashvm/styxrpc_graph.h"


/* EN: Consume fd and publish FlashVM graph functions on the owner thread.
 * RU: Принять fd и опубликовать функции графов в потоке владельца FlashVM. */
int test_graph_serve(FVM_Context *context, int fd);


/* EN: Run the Dis graph client and disarm its target before leaving.
 * RU: Запустить графовый клиент Dis и отключить target перед выходом. */
int test_graph_call(FVM_Context *context, StyxRpcManagedTarget *target);
#endif
