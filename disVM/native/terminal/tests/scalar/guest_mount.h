/* Copyright (c) 2026 bhgv
 * EN: Test-only DisVM bootstrap with live Styx mounts.
 * RU: Запуск DisVM с живыми монтированиями Styx для тестов.
 */
#ifndef STYX_SCALAR_GUEST_MOUNT_H
#define STYX_SCALAR_GUEST_MOUNT_H
#include <stddef.h>
typedef struct DVM_Provider DVM_Provider; /* Borrowed mounted file. */


/* EN: Run CLI argc/argv once, borrowing the local reader and remote writer.
 * owner selects which path is readable. Return the real runtime status.
 * RU: Запустить CLI argc/argv, заимствуя локальное чтение и удалённую запись.
 * owner выбирает доступный для чтения путь. Вернуть код runtime. */
int scalar_runtime_run(int argc, char **argv, DVM_Provider *writer, DVM_Provider *reader, DVM_Provider *original, DVM_Provider *profile, int owner);

/* EN/RU: Return an owned inbox seeded from the fixture's compiled signature.
 * Вернуть собственный приёмник с компилированной сигнатурой теста. */
DVM_Provider *scalar_local_profile(void);

/* EN/RU: Read a borrowed fixture path into an owned image provider.
 * Прочитать тестовый путь в собственный провайдер образа. */
DVM_Provider *scalar_local_image(const char *path);

/* EN/RU: Borrow a plain peer mount for one VM invocation.
 * Заимствовать обычный монт другой стороны на время одного запуска VM. */
int scalar_runtime_mounted(int argc, char **argv, DVM_Provider *mount);

/* EN/RU: Borrow two request/reply pairs through one owner VM invocation.
 * Заимствовать две пары запросов и ответов на время работы VM владельца. */
int scalar_runtime_shared(int argc, char **argv, DVM_Provider *const *requests, DVM_Provider *const *replies);

/* EN: Borrow the server routes for one automatic loaded set.
 * RU: Заимствовать маршруты сервера для одного автоматически связанного набора. */
int scalar_runtime_loaded_set(int argc, char **argv, const char *implementation, const char *second,
    const char *const *names, size_t count,
    DVM_Provider *const *requests, DVM_Provider *const *replies, DVM_Provider *const *offers);

#endif
