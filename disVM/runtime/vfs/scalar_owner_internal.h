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
#endif
