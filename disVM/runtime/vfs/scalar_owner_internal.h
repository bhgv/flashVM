/* Copyright (c) 2026 bhgv
 * EN: Runtime-private helpers for scalar owner entry points.
 * RU: Внутренние функции для скалярных входов владельца.
 */
#ifndef DVM_SCALAR_OWNER_INTERNAL_H
#define DVM_SCALAR_OWNER_INTERNAL_H

/* EN: Copy a bounded guest path. / RU: Скопировать гостевой путь. */
int dvm_scalar_copy_path(String *name, char output[DVM_NS_PATH_LIMIT]);

/* EN: Send a signed result through an owner mailbox.
 * RU: Отправить знаковый результат через приёмник владельца. */
void dvm_scalar_owner_reply(String *name, int64_t tag, WORD success, int64_t value);
/* EN: Publish one exact native record and return one tuple request respectively.
 * RU: Отправить точную запись и получить один запрос кортежа соответственно. */
void dvm_scalar_owner_record(String *path, const void *bytes, size_t size);
typedef int (*DVM_OwnerDecode)(unsigned long, const void *, size_t, void *); /* Decode one complete native record. */


/* EN: Read one typed record from path, using decoder to stage decoded. Return
 * 1 on success, 0 at EOF and raise on failure after releasing native wait.
 * RU: Прочитать типизированную запись из path, применить decoder к decoded.
 * Вернуть 1 при успехе, 0 при EOF. Ошибка возникает после нативного ожидания. */
int dvm_scalar_owner_next(String *path, unsigned long tag, DVM_OwnerDecode decoder, void *decoded);
/* EN: Link the named local export with the required Dis signature into ret.
 * RU: Связать именованный локальный экспорт по сигнатуре Dis и записать в ret. */
void dvm_scalar_load_composed(String *path, String *name, uint32_t signature, Modlink **ret);
#endif
