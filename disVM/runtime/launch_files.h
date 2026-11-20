/* Copyright (c) 2026 bhgv
 * EN: Launch-owned numbered file table and native-only suffix of builtin Sys.FD.
 * RU: Номерная таблица файлов запуска и нативное расширение builtin Sys.FD.
 */
#ifndef DVM_RUNTIME_LAUNCH_FILES_H
#define DVM_RUNTIME_LAUNCH_FILES_H
#include "file_slots.h"
#include "vfs/launch/ticket.h"

typedef struct DVM_LaunchFiles DVM_LaunchFiles; /* Native refcounted table, inherited by launch descendants. */


/* EN/RU: Public Sys.FD layout stays at offset zero. Only TFD objects have this suffix.
 * Публичный Sys.FD остаётся по смещению ноль. Суффикс есть только у объектов TFD. */
typedef struct DVM_SysFD
{
    Sys_FD public_fd; /* Limbo-visible descriptor number. */
    int launch_slot;  /* Owned internal shared slot; zero for ordinary objects. */
} DVM_SysFD;


/* EN/RU: Discover and open the launch's numbered files before dispatch; success returns
 * an owned table or NULL if empty. Explicit terminal modes override discovery.
 * On failure return zero and retire any obtained files. Handoff is borrowed.
 * Найти и открыть номерные файлы до dispatch; успех даёт свою таблицу или NULL.
 * Явно заданные режимы терминала заменяют автоматическое обнаружение.
 * При отказе вернуть ноль и закрыть полученные файлы. Handoff заимствован. */
int dvm_launch_files_open(const DVM_LaunchHandoff *handoff, DVM_LaunchFiles **out);


/* EN/RU: Retain a borrowed table (NULL allowed), return 1 or zero on overflow.
 * Удержать заимствованную таблицу (NULL допустим), вернуть 1 или ноль при переполнении. */
int dvm_launch_files_retain(DVM_LaunchFiles *files);


/* EN/RU: Drop one table pin; release file slots on the last pin; NULL allowed.
 * Освободить ссылку на таблицу; последняя освобождает слоты; NULL допустим. */
void dvm_launch_files_release(DVM_LaunchFiles *files);


/* EN/RU: Borrow the current process's imported slot, or -1 if the guest number is absent.
 * Найти заимствованный импортированный слот процесса, либо -1 при отсутствии номера. */
int dvm_launch_file_current(int number);


/* EN/RU: Return the occupied or intentionally hidden guest numbers in the current launch, or zero.
 * Вернуть маску переданных или явно скрытых номеров текущего запуска, иначе ноль. */
uint32_t dvm_launch_file_mask(void);


/* EN/RU: Return 1 if the current launch explicitly hides this number.
 * Вернуть 1, если текущий запуск явно скрывает этот номер. */
int dvm_launch_file_hidden(unsigned number);


/* EN/RU: Return the internal slot of a validated, live builtin FD, or -1 for an invalid public number.
 * Вернуть внутренний слот проверенного живого builtin FD, либо -1 при неверном номере. */
int dvm_sys_fd_slot(Sys_FD *fd);

#endif
