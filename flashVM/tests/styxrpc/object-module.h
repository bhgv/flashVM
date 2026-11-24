/* Copyright (c) 2026 bhgv
 * EN: Integration entry points for callable FlashVM and Dis module objects.
 * RU: Сквозные точки проверки вызываемых объектов модулей FlashVM и Dis.
 */
#ifndef FVM_TEST_STYXRPC_OBJECT_MODULE_H
#define FVM_TEST_STYXRPC_OBJECT_MODULE_H
#include "flashvm/flashvm.h"

struct StyxClient; /* Attached test session. */

/* EN: Serve a live image on fd. / RU: Обслужить живой образ на fd. */
int test_object_image_serve(FVM_Context *context, int fd);

/* EN: Load and call on session. / RU: Загрузить и вызвать через session. */
int test_object_module_call(FVM_Context *context, struct StyxClient *session);
#endif
