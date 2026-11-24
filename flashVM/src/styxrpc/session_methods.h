/* Copyright (c) 2026 bhgv
 * EN: Internal native members on one hosted ActionScript session object.
 * RU: Внутренние нативные методы объекта хостовой сессии ActionScript.
 */
#ifndef FLASHVM_STYXRPC_SESSION_METHODS_H
#define FLASHVM_STYXRPC_SESSION_METHODS_H
#include "flashvm/styxrpc_session.h"


/* EN: Add named launch methods to a rooted, unpublished object.
 * RU: Добавить методы запуска к удерживаемому неопубликованному объекту. */
FVM_Result fvm_styxrpc_launch_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Add file I/O methods to a rooted, unpublished object.
 * RU: Добавить методы файлов к удерживаемому неопубликованному объекту. */
FVM_Result fvm_styxrpc_vfs_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding);


/* EN: Copy a nonempty counted string to a NUL-terminated native name.
 * RU: Скопировать непустую строку с длиной в нуль-терминированное имя. */
int fvm_styxrpc_name(const FVM_Value *value, char *name, size_t capacity);


/* EN: Admit an exact nonnegative AS integer within the supplied range.
 * RU: Принять точное неотрицательное целое AS в заданном диапазоне. */
int fvm_styxrpc_number(const FVM_Value *value, uint64_t limit, uint64_t *result);
#endif
