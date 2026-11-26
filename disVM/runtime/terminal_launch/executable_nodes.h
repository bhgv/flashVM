/* Copyright (c) 2026 bhgv
 * EN: Publish a session-scoped Limbo execute broker and its two file views.
 * RU: Опубликовать брокер исполнения Limbo и два его файловых представления.
 */
#ifndef DVM_EXECUTABLE_NODES_H
#define DVM_EXECUTABLE_NODES_H
#include "../vfs/resource.h"

typedef struct DvmExecBroker DvmExecBroker; /* Native-only session state. */


/* EN: Install the local owner file. The caller pins group until stop and join.
 * RU: Установить локальный файл владельца. Вызывающий удерживает group до stop и join. */
DvmExecBroker *dvm_exec_install(DVM_Namespace *group);


/* EN: Return a new owned peer file provider for the borrowed broker.
 * RU: Вернуть файловый провайдер для клиента, удерживающий брокер. */
DVM_Provider *dvm_exec_service(DvmExecBroker *broker);


/* EN: Stop admissions and wake the dispatcher, then release the launcher's reference.
 * RU: Остановить приём и разбудить диспетчер, затем освободить ссылку запуска. */
void dvm_exec_stop(DvmExecBroker *broker);

#endif
