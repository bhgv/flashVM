/* Copyright (c) 2026 bhgv
 * EN: Native descriptor registry; rooted callers retain all borrowed lifetimes.
 * RU: Реестр нативных дескрипторов; укоренённые вызывающие держат время жизни.
 */
#ifndef DVM_FILE_SLOTS_H
#define DVM_FILE_SLOTS_H
#include "file_gate.h"
#include "close_worker.h"


/* EN: One reserved/open descriptor's detached native ownership, without VM roots.
 * RU: Отделённое владение одним занятым/открытым дескриптором, без корней VM. */
typedef struct DVM_FileSlot
{
    DVM_FileGate *gate; /* Reservation identity, non-NULL until detachment. */
    DVM_CloseJob *job; /* Preallocated retirement node, owned with the gate. */
    FVM_PortFile *file; /* Published native file, or NULL. */
    FVM_PortDirectory *directory; /* Published native directory, or NULL. */
} DVM_FileSlot;


/* EN: Reserve a unique slot for non-NULL gate/job under the registry mutex.
 * Transfer ownership on success (slot 3..31); return -1 with inputs untouched
 * when full. No allocation, port call or GC access occurs inside the mutex.
 * RU: Занять уникальный слот для ненулевых gate/job под мьютексом реестра.
 * Передать владение при успехе (3..31); при заполнении вернуть -1 без передачи.
 * В мьютексе нет выделения памяти, портовых вызовов или доступа к GC. */
int dvm_file_slots_reserve(DVM_FileGate *gate, DVM_CloseJob *job);


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
#endif
