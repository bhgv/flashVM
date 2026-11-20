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

/* EN/RU: Pair an exact read-only original node with a trusted local adapter.
 * Связать исходный узел только для чтения с доверенным локальным адаптером. */
DVM_Provider *dvm_module_publish(DVM_Provider *original, DVM_Provider *adapter);
/* EN/RU: Also require an exact cancellable profile file on typed load.
 * Также требовать точный файл профиля с отменой при типизированном load. */
DVM_Provider *dvm_module_publish_profile(DVM_Provider *original, DVM_Provider *adapter,
    DVM_Provider *profile, unsigned long tag);
/* EN/RU: Publish one remote fn(int):int without a local bytecode adapter.
 * Опубликовать удалённую fn(int):int без локального адаптера Dis. */
DVM_Provider *dvm_module_publish_scalar(DVM_Provider *original, DVM_Provider *profile,
    unsigned long profile_tag, unsigned long request_tag, unsigned long reply_tag);
/* EN/RU: Publish scalar library with exact borrowed route grants captured on success.
 * Опубликовать скалярную библиотеку с захваченными файлами маршрута. */
DVM_Provider *dvm_module_publish_scalar_files(DVM_Provider *original, DVM_Provider *profile,
    unsigned long profile_tag, DVM_Provider *request, unsigned long request_tag,
    DVM_Provider *reply, unsigned long reply_tag);
#endif
