/* Copyright (c) 2026 bhgv
 * EN: Native descriptor registry; rooted callers retain all borrowed lifetimes.
 * RU: Реестр нативных дескрипторов; укоренённые вызывающие держат время жизни.
 */
#ifndef DVM_FILE_SLOTS_H
#define DVM_FILE_SLOTS_H
#include "file_gate.h"
#include "close_worker.h"
#include "vfs/handle.h"
#include <stdint.h>

#define DVM_FILE_SLOT_LIMIT 64 /* Ordinary slots 3..31, retained launch files 32..63. */
#define DVM_LAUNCH_SLOT_FIRST 32 /* First internal slot for a shared launch file. */

/* EN: One reserved/open descriptor's detached native ownership, without VM roots.
 * RU: Отделённое владение одним занятым/открытым дескриптором, без корней VM. */
typedef struct DVM_FileSlot
{
    DVM_FileGate *gate; /* Reservation identity, non-NULL until detachment. */
    DVM_CloseJob *job; /* Preallocated retirement node, owned with the gate. */
    DVM_VfsHandle resource; /* Published native ownership, empty until open succeeds. */
} DVM_FileSlot;


/* EN: Reserve a unique slot for non-NULL gate/job under the registry mutex.
 * Transfer ownership on success (slot 3..31); return -1 with inputs untouched
 * when full. No allocation, port call or GC access occurs inside the mutex.
 * RU: Занять уникальный слот для ненулевых gate/job под мьютексом реестра.
 * Передать владение при успехе (3..31); при заполнении вернуть -1 без передачи.
 * В мьютексе нет выделения памяти, портовых вызовов или доступа к GC. */
int dvm_file_slots_reserve(DVM_FileGate *gate, DVM_CloseJob *job);


/* EN/RU: Reserve an ordinary slot excluding the caller's imported guest numbers.
 * Зарезервировать обычный слот, исключив гостевые номера этого запуска. */
int dvm_file_slots_reserve_masked(DVM_FileGate *gate, DVM_CloseJob *job, uint32_t occupied);


/* EN/RU: Reserve a shared launch slot with one reference; consume gate/job on success only.
 * Зарезервировать слот запуска с одной ссылкой; принять gate/job только при успехе. */
int dvm_file_slots_reserve_launch(DVM_FileGate *gate, DVM_CloseJob *job);


/* EN/RU: Retain a published launch slot under a live reference; return 1/0.
 * Удержать опубликованный слот запуска при живой ссылке; вернуть 1/0. */
int dvm_file_slots_retain_launch(int descriptor);


/* EN/RU: Drop an imported or exported pin; the last queues native close.
 * Освободить ссылку импорта или экспорта; последняя ставит закрытие в очередь. */
void dvm_file_slots_drop_launch(int descriptor);


/* EN/RU: Allocate a never-reused group identity, or zero on exhaustion.
 * Выдать неповторимый идентификатор группы либо ноль при исчерпании. */
uint64_t dvm_file_slots_new_group(void);


/* EN: Associate a newly opened ordinary descriptor with its live guest owner and mode.
 * The caller exclusively owns the published descriptor and passes a nonzero group ID.
 * RU: Связать новый обычный дескриптор с владельцем и режимом. У вызывающего
 * исключительное владение опубликованным дескриптором и ненулевой ID группы. */
void dvm_file_slots_mark_guest(int descriptor, uint64_t group, unsigned access);


/* EN: Pin a live ordinary file in the specified group and copy its open mode.
 * Returns 1 when pinned, 0 on absence, -1 on reference overflow.
 * RU: Удержать обычный файл заданной группы и скопировать режим открытия.
 * Возвращает 1 при успехе, 0 при отсутствии, -1 при переполнении. */
int dvm_file_slots_pin_guest(int descriptor, uint64_t group, unsigned *access);


/* EN: Remove an ordinary file from discovery and drop its local owner.
 * Return 1 if consumed, 0 for slots outside guest ownership.
 * RU: Убрать обычный файл из выдачи и снять ссылку владельца.
 * Вернуть 1 при освобождении, иначе 0. */
int dvm_file_slots_close_guest(int descriptor);


/* EN: Publish file/directory into the caller's exclusively reserved descriptor.
 * Transfer native handle ownership; exactly one handle must be non-NULL.
 * Invalid reservation is a fatal internal contract error, never a VM exception.
 * RU: Опубликовать file/directory в принадлежащем вызывающему резерве descriptor.
 * Передать владение; ровно один handle ненулевой. Неверная резервация —
 * фатальное нарушение внутреннего контракта, не исключение VM. */
void dvm_file_slots_publish(int descriptor, FVM_PortFile *file, FVM_PortDirectory *directory);


/* EN: Detach the caller's exclusive slot into output, leaving it reusable.
 * published rejects unfinished reservations when nonzero. Return 1 on transfer,
 * 0 if absent; output is non-NULL writable native storage, unchanged on refusal. Caller retires handles outside
 * the mutex and guarantees no existing/future borrowers (FD roots or loader).
 * RU: Отделить личный слот в output, освободив номер для повторного применения.
 * published запрещает незавершённый резерв. Вернуть 1 при передаче, иначе 0,
 * не меняя output. Вызывающий закрывает ресурсы вне мьютекса и гарантирует
 * отсутствие заёмщиков (корни FD либо владение загрузчика). */
int dvm_file_slots_take(int descriptor, int published, DVM_FileSlot *output);


/* EN: Borrow descriptor's file while a rooted FD or exclusive loader pins it.
 * Return NULL for invalid/non-file slots; lookup locks, without extending lifetime.
 * RU: Заимствовать файл descriptor, удерживаемый корнем FD либо загрузчиком.
 * Для неверного/не-файлового слота вернуть NULL; поиск не продлевает жизнь. */
FVM_PortFile *dvm_runtime_file(int descriptor);


/* EN: Borrow descriptor's directory with the same root/loader lifetime contract.
 * Return NULL for invalid/non-directory slots; lookup uses the registry mutex.
 * RU: Заимствовать каталог descriptor с тем же удержанием корнем/загрузчиком.
 * Для неверного/не-каталожного слота вернуть NULL; поиск под мьютексом реестра. */
FVM_PortDirectory *dvm_runtime_directory(int descriptor);


/* EN/RU: Borrow a live provider handle under rooted FD/loader lifetime; NULL for
 * invalid/reserved slots. No new ownership or operation admission is granted.
 * Заимствовать provider handle с rooted FD/loader lifetime; NULL для резерва
 * или invalid slot. Владение и operation admission не предоставляются. */
DVM_VfsHandle *dvm_runtime_resource(int descriptor);

/* EN/RU: Move trusted native provider into caller's reserved slot, clearing source.
 * Allocation/validation precede publication; invalid ownership is fatal.
 * No callback runs under registry lock. Перенести provider в свой reserved slot,
 * очистить source; invalid ownership fatal. Без callback под registry lock. */
void dvm_file_slots_publish_resource(int descriptor, DVM_VfsHandle *resource);
#endif
