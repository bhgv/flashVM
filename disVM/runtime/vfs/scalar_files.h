/* Copyright (c) 2026 bhgv
 * EN: Shared exact Styx route retained by a publication and its linked modules.
 * RU: Точный маршрут Styx, удерживаемый публикацией и загруженными модулями.
 */
#ifndef DVM_SCALAR_FILES_H
#define DVM_SCALAR_FILES_H
#include "provider.h"
#include "close_worker.h"

typedef struct DVM_ScalarFiles
{
    DVM_Provider *request; /* Owned captured request node. */
    DVM_Provider *reply; /* Owned captured reply node. */
    DVM_CloseJob *close; /* Native retirement reservation. */
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

#endif
