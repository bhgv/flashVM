/* Copyright (c) 2026 bhgv
 * EN: Native worker handle registry, isolated from managed heap admission.
 * RU: Реестр нативных потоков, независимый от допуска к управляемой куче.
 */
#ifndef DVM_THREAD_REGISTRAR_H
#define DVM_THREAD_REGISTRAR_H

#include "thread_port.h"

typedef struct DVM_ThreadRegistrar DVM_ThreadRegistrar;
typedef struct DVM_ThreadSlot DVM_ThreadSlot;

/* EN: Start an empty native registry. Null means startup failed.
 * RU: Запустить пустой реестр потоков. Null означает ошибку запуска. */
DVM_ThreadRegistrar *dvm_thread_registrar_create(void);

/* EN: Stop the registry once all worker slots have been joined.
 * RU: Остановить реестр после присоединения всех рабочих потоков. */
void dvm_thread_registrar_destroy(DVM_ThreadRegistrar *registrar);

/* EN: Ask the registrar to start fn(arg); null means no worker was started.
 * RU: Попросить регистратор запустить fn(arg). При отказе вернуть null. */
DVM_ThreadSlot *dvm_thread_registrar_start(DVM_ThreadRegistrar *registrar,
    FVM_PortThreadFn fn, void *arg);

/* EN: Join and release slot. The caller must let the worker finish first and
 * must not hold admission the worker needs. Returns zero on delivery failure.
 * RU: Присоединить и освободить slot. Поток должен иметь возможность завершиться.
 * Вызывающий не удерживает нужный работнику допуск. При отказе доставки вернуть ноль. */
int dvm_thread_registrar_join(DVM_ThreadRegistrar *registrar, DVM_ThreadSlot *slot);

#endif
