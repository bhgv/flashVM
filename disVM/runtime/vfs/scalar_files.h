/* Copyright (c) 2026 bhgv
 * EN: Shared exact Styx route retained by a publication and its linked modules.
 * RU: Точный маршрут Styx, удерживаемый публикацией и загруженными модулями.
 */
#ifndef DVM_SCALAR_FILES_H
#define DVM_SCALAR_FILES_H
#include "provider.h"
#include "namespace.h"
#include <stdint.h>
#include "close_worker.h"
#include "../../native/terminal/scalar_verify.h"
#include "../../native/terminal/tuple_record.h"

typedef struct DVM_ScalarFiles
{
    struct DVM_ScalarFiles *next; /* Owned next mounted export, in compiled import order. */
    char name[INTERFACE_NAME_LIMIT + 1]; /* Mounted export name. */
    DVM_Provider *request; /* Owned captured request node. */
    DVM_Provider *reply; /* Owned captured reply node. */
    DVM_CloseJob *close; /* Native retirement reservation. */
    uint32_t signature; /* Verified compiled import for this exact function. */
    int caller; /* Frame includes a captured caller PID. */
    int duplex; /* One fresh read/write fid per call. */
    unsigned references; /* Atomic native ownership. */
    unsigned long state; /* Shared outstanding/retired exchange state. */
} DVM_ScalarFiles;


/* EN/RU: Capture two exact borrowed file grants; owned result or NULL.
 * Захватить два заимствованных файла. Вернуть собственную ссылку или NULL. */
DVM_ScalarFiles *dvm_scalar_files(DVM_Provider *request, DVM_Provider *reply);


/* EN/RU: Retain a live optional route; NULL succeeds.
 * Удержать живой необязательный маршрут. NULL допустим. */
int dvm_scalar_files_retain(DVM_ScalarFiles *files);


/* EN/RU: Release one ref and defer the final provider destruction.
 * Снять одну ссылку, отложив уничтожение провайдеров. */
void dvm_scalar_files_release(DVM_ScalarFiles *files);

/* EN/RU: Capture a function in a selected mounted module directory.
 * Захватить функцию в выбранном каталоге смонтированного модуля. */
DVM_ScalarFiles *dvm_scalar_function_files(DVM_Provider *directory, const char *name);

/* EN/RU: Exchange on one fid; returns 1 for value, 2 for owner error, 0 for unknown, -1 before submission.
 * Обмен через один fid. Возврат 1 для значения, 2 для ошибки владельца, 0 при неизвестном исходе, -1 до отправки. */
int dvm_scalar_function_call(DVM_ScalarFiles *files, uint32_t pid, int64_t argument, int64_t *value);

/* EN/RU: Run a scalar call with the optional native reverse publication binding.
 * Выполнить скалярный вызов с необязательной связью встречной публикации. */
struct DVM_LaunchFiles;
int dvm_scalar_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    struct DVM_LaunchFiles *descriptors, uint32_t pid,
    int64_t argument, int64_t *value);

/* EN: Exchange one composed value using the scoped route when configured.
 * RU: Обменять составное значение через настроенный маршрут. */
int dvm_tuple_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    struct DVM_LaunchFiles *descriptors, uint32_t pid, const StyxTuple *argument, StyxTuple *result);
/* EN: Exchange one ref snapshot and stage the return and copy-back.
 * RU: Обменять снимок ref и подготовить результат и обновление. */
int dvm_ref_function_invoke(DVM_ScalarFiles *files, DVM_Namespace *group,
    struct DVM_LaunchFiles *descriptors, uint32_t pid, const StyxTuple *argument, StyxRefReply *result);
#endif
