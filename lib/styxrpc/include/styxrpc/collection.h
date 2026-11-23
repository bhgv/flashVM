/* Copyright (c) 2026 bhgv
 * EN: Register a live indexed Int32 collection in the process's .var directory.
 * RU: Регистрация живой индексируемой коллекции Int32 в .var процесса.
 */
#ifndef STYXRPC_COLLECTION_H
#define STYXRPC_COLLECTION_H
#include "styxrpc/program.h"
#include "styxrpc/indexed_variable.h"

#ifdef __cplusplus
extern "C" {
#endif


/* EN: Publish .idx/.val/.len/.type and optional mutation files as one registration.
 * Keep collection state and callbacks alive through all fids; use a zeroed selector state.
 * Positional arrays use insert/pop. Keyed collections use contains/add.
 * RU: Опубликовать .idx/.val/.len/.type и доступные мутации одной регистрацией.
 * Поддерживать время жизни состояния и колбэков до закрытия fid. Состояние выбора обнулить.
 * Позиционный массив использует insert/pop, коллекция с ключами — contains/add. */
int styxrpc_program_collection(StyxRpcPrograms *programs, uint32_t pid, const char *name, StyxIntArray *array);

#ifdef __cplusplus
}
#endif
#endif
