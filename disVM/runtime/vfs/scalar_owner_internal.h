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
void dvm_scalar_owner_record(String *path, const void *bytes, size_t size, int scoped);
typedef int (*DVM_OwnerDecode)(unsigned long, const void *, size_t, void *); /* Decode one complete native record. */


/* EN: Decode one record from path. Return 1 on success, 0 at EOF and
 * raise on transport failure. Scoped reads may return -1 if installation fails.
 * RU: Декодировать одну запись из path. Вернуть 1 при успехе, 0 при EOF.
 * Ошибка транспорта вызывает исключение. При отказе установки scoped вернёт -1. */
int dvm_scalar_owner_next(String *path, unsigned long tag, DVM_OwnerDecode decoder, void *decoded, int scoped);


/* EN: Install a private namespace and event inbox from the last read on handle.
 * RU: Установить частный namespace и очередь событий из последнего чтения handle. */
int dvm_scalar_caller_begin(Prog *process, DVM_Namespace *group, DVM_VfsHandle *handle);


/* EN: Retire the private scope before replying or on cancellation.
 * RU: Завершить частный контекст до ответа либо при отмене. */
void dvm_scalar_caller_end(Prog *process);

/* EN: Reserve the current private call for a dispatcher spawn. Return 1 if armed.
 * RU: Зарезервировать частный вызов для spawn диспетчера. Вернуть 1 при успехе. */
int dvm_scalar_arm_handoff(void);
void dvm_scalar_discard_staged(Prog *process);
int dvm_scalar_take_staged(Prog *parent, Prog *child);


/* EN: Decode an ordinary or locally wrapped request. Mark context as required for a wrapper.
 * RU: Декодировать обычный или обёрнутый локальный запрос. Оболочка требует контекст. */
int dvm_scalar_owner_decode(DVM_OwnerDecode decode, unsigned long tag, const void *bytes, size_t size,
    void *decoded, int *required);
/* EN: Link the named local export with the required Dis signature into ret.
 * RU: Связать именованный локальный экспорт по сигнатуре Dis и записать в ret. */
void dvm_scalar_load_composed(String *path, String *name, uint32_t signature, Modlink **ret);
#endif
