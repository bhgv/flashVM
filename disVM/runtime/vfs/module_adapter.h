/* Copyright (c) 2026 bhgv
 * EN: Exact executable VFS node for trusted, locally built Dis adapters.
 * RU: Исполняемый узел VFS для доверенных локально собранных адаптеров Dis.
 */
#ifndef DVM_MODULE_ADAPTER_H
#define DVM_MODULE_ADAPTER_H
#include "provider.h"


/* EN: Copy local image bytes into an immutable exact-root provider, or return NULL.
 * Native publisher supplies trusted bytes. Caller owns the returned provider.
 * RU: Скопировать доверенный локальный образ в неизменяемый провайдер, либо NULL.
 * Вызывающий владеет возвращённым провайдером. */
DVM_Provider *dvm_module_adapter(const void *bytes, size_t size);

#endif
