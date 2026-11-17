/* Copyright (c) 2026 bhgv
 * EN: Portable IDE UI frame codec shared by local queues and bidir terminal routes.
 * RU: Portable IDE UI frame codec для local queues и bidir terminal routes.
 */
#ifndef DISVM_UI_FRAME_H
#define DISVM_UI_FRAME_H
#include "ui_message.h"
#include "flashvm/flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif


/* EN/RU: Encode copied UI values into borrowed nonoverlapping storage; OK sets used, failure preserves it.
 * No allocation, VM/graphics/channel or I/O. Canonical fvd1 and existing bounds remain unchanged.
 * Кодировать values в borrowed nonoverlapping storage; used только при OK. Без allocation/VM/I/O. */
FVM_Result dvm_ui_frame_encode(const DVM_UIMessage *message, char *storage, size_t capacity, size_t *used);


/* EN/RU: Decode borrowed counted frame to copied message; output unchanged on failure.
 * Canonical UTF-8/header/int32 validation; no allocation, native pointers or dispatch.
 * Декодировать counted frame в copied message; output неизменен при отказе, без allocation/dispatch. */
FVM_Result dvm_ui_frame_decode(const void *bytes, size_t size, DVM_UIMessage *message);
#ifdef __cplusplus
}
#endif
#endif
