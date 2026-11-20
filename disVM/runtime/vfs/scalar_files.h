/* Copyright (c) 2026 bhgv
 * EN: Shared exact Styx route retained by a publication and its linked modules.
 * RU: Точный маршрут Styx, удерживаемый публикацией и загруженными модулями.
 */
#ifndef DVM_SCALAR_FILES_H
#define DVM_SCALAR_FILES_H
#include "provider.h"
#include <stdint.h>
#include "close_worker.h"

typedef struct DVM_ScalarFiles
{
    DVM_Provider *request; /* Owned captured request node. */
    DVM_Provider *reply; /* Owned captured reply node. */
    DVM_CloseJob *close; /* Native retirement reservation. */
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
int dvm_scalar_function_call(DVM_ScalarFiles *files, int64_t argument, int64_t *value);

#endif
