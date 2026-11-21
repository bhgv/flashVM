/* Copyright (c) 2026 bhgv
 * EN: Runtime-private helpers for scalar owner entry points.
 * RU: Внутренние функции для скалярных входов владельца.
 */
#ifndef DVM_SCALAR_OWNER_INTERNAL_H
#define DVM_SCALAR_OWNER_INTERNAL_H
#include "provider.h"

/* EN: Copy a bounded guest path. / RU: Скопировать гостевой путь. */
int dvm_scalar_copy_path(String *name, char output[DVM_NS_PATH_LIMIT]);

/* EN: Send a signed result through an owner mailbox.
 * RU: Отправить знаковый результат через приёмник владельца. */
void dvm_scalar_owner_reply(String *name, int64_t tag, WORD success, int64_t value);
/* EN: Publish one exact native record and return one tuple request respectively.
 * RU: Отправить точную запись и получить один запрос кортежа соответственно. */
void dvm_scalar_owner_record(String *path, const void *bytes, size_t size);
typedef int (*DVM_OwnerDecode)(unsigned long, const void *, size_t, void *); /* Decode one complete native record. */


/* EN: Decode one record from path. Return 1 on success, 0 at EOF and
 * raise on transport failure. Scoped reads may return -1 if installation fails.
 * RU: Декодировать одну запись из path. Вернуть 1 при успехе, 0 при EOF.
 * Ошибка транспорта вызывает исключение. При отказе установки scoped вернёт -1. */
int dvm_scalar_owner_next(String *path, unsigned long tag, DVM_OwnerDecode decoder, void *decoded, int scoped);


/* EN: Build a private copy of group exposing context at /.prog/.self.
 * RU: Создать частную копию group с context в /.prog/.self. */
DVM_Namespace *dvm_scalar_caller_namespace(DVM_Namespace *group, DVM_Provider *context);
/* EN: Link the named local export with the required Dis signature into ret.
 * RU: Связать именованный локальный экспорт по сигнатуре Dis и записать в ret. */
void dvm_scalar_load_composed(String *path, String *name, uint32_t signature, Modlink **ret);
#endif
