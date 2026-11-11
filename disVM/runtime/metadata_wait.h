/* Copyright (c) 2026 bhgv
 * EN: Typed native metadata waits with rooted callers and owner-local errors.
 * RU: Типизированные ожидания метаданных с корнями вызывающего и личными ошибками.
 * Caller includes the generated native runt.h.
 */
#ifndef DVM_METADATA_WAIT_H
#define DVM_METADATA_WAIT_H
#include "filesystem_port.h"


/* Copy borrowed validated UTF-8 path into native storage before releasing the
 * ACTIVE heap turn. output receives copied metadata only on success. No managed
 * pointer is followed while suspended; native staging is freed after reentry. */
FVM_Result dvm_metadata_path_info(const char *path, FVM_FileInfo *output);


/* Borrow the mapped FD and checked descriptor while ACTIVE, pin FD, reserve its
 * file/directory gate, release heap ownership, query native metadata, finish gate
 * BEFORE reentry and unpin. output changes only on success. Invalid slots return
 * an explicit status; root-registration errors use the ordinary managed unwind.
 * Cancellation returns FVM_ERROR with its TLS reason; only allocation failure
 * may return FVM_OUT_OF_MEMORY. Caller must preserve the adapter diagnostic.
 * RU: Отмена возвращает FVM_ERROR с причиной в TLS; нехватка памяти относится
 * только к выделению. Вызывающий сохраняет диагностику адаптера. */
FVM_Result dvm_metadata_fd_info(Sys_FD *fd, int descriptor, FVM_FileInfo *output);


/* Read a bounded batch, then publish it on the resident while still owning
 * the cursor ticket. entries/count are borrowed by publish, which may raise.
 * The adapter frees the batch and releases the ticket on every path.
 * Cancellation before admission returns FVM_ERROR, preserves entries/count,
 * sets the TLS reason and never calls publish. Preserve that diagnostic.
 * RU: Отмена до допуска возвращает FVM_ERROR, сохраняет entries/count,
 * записывает причину в TLS и не вызывает publish. Не затирать эту диагностику. */
FVM_Result dvm_metadata_directory_read(Sys_FD *fd, int descriptor, FVM_FileInfo **entries,
    size_t *count, int (*publish)(void *), void *argument);


/* EN: Print a mutex-protected native counter snapshot, normally after join.
 * RU: Вывести снимок нативных счётчиков под мьютексом, обычно после join. */
void dvm_metadata_statistics(void);
#endif
