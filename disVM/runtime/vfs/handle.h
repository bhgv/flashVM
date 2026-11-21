/* Copyright (c) 2026 bhgv
 * EN: Native provider dispatch and linear resource ownership; no managed pointers.
 * RU: Native provider dispatch и линейное владение; без managed pointers.
 */
#ifndef DVM_VFS_HANDLE_H
#define DVM_VFS_HANDLE_H
#include "file_port.h"
#include "filesystem_port.h"
#include "io_result.h"

#define DVM_VFS_CANCEL_INFO 2U /* Held-fid metadata consumes current cancellation scope. */
#define DVM_VFS_CANCEL_SEEK 4U /* Seek, including metadata query, consumes current cancellation scope. */
#define DVM_VFS_CANCEL_LIST 8U /* Directory enumeration consumes current cancellation scope. */
#define DVM_VFS_CANCEL_CLOSE 16U /* Consuming native close honours the current cancellation source, including pre-cancel. */
#define DVM_VFS_CANCEL_MASK 31U /* Known immutable handle cancellation capabilities. */
#define DVM_VFS_CANCEL_DATA 1U /* Native read/write consumes current cancel scope; not directory/control I/O. */


/* EN/RU: Trusted immutable native callbacks. State is borrowed for each serialized
 * call; close consumes it. No managed access or guest callbacks are allowed.
 * Доверенные immutable callbacks; state заимствован на serialized call, close
 * поглощает его. Managed access и guest callbacks запрещены. */
struct DVM_Provider; /* Optional native context owned by a completed local record. */
typedef struct DVM_VfsOps
{
    FVM_Result (*info)(void *state, FVM_FileInfo *output); /* Optional info; NULL returns UNSUPPORTED. */
    FVM_Result (*read)(void *state, void *bytes, size_t size, size_t *received); /* Optional read; NULL returns UNSUPPORTED. */
    FVM_Result (*write)(void *state, const void *bytes, size_t size); /* Optional write; NULL returns UNSUPPORTED. */
    FVM_Result (*seek)(void *state, long offset, int origin, long *position); /* Optional seek; NULL returns UNSUPPORTED. */
    FVM_Result (*list)(void *state, FVM_FileInfo **entries, size_t *count); /* Optional list; NULL returns UNSUPPORTED. */
    void (*close)(void *state); /* Mandatory consuming native close, possibly blocking. */
    void (*abort_list)(void *state); /* Required with list: invalidate cursor after publication failure. */
    void (*read_chunk)(void *state, void *bytes, size_t size, DVM_IoResult *result); /* Optional precise read acknowledgement. */
    void (*write_chunk)(void *state, const void *bytes, size_t size, DVM_IoResult *result); /* Optional precise write acknowledgement. */
    struct DVM_Provider *(*take_context)(void *state); /* Transfer the context of the last consumed record once. */
} DVM_VfsOps;


/* EN/RU: One owning value, never copy a live handle. Borrow under an existing FD
 * lifetime pin and FIFO; move only after all borrowers retire. No internal lock.
 * Один owning value; живой handle не копировать. Заимствовать с FD pin/FIFO;
 * переносить после ухода заёмщиков. Внутреннего lock нет. */
typedef struct DVM_VfsHandle
{
    const DVM_VfsOps *ops; /* Immutable provider contract, NULL for empty handle. */
    void *state; /* Exclusive native ownership, provider-defined; never a VM pointer. */
    DVM_IoResult last_io; /* Last synchronous chunk outcome under the existing caller FIFO. */
    int io_valid; /* Last outcome exists; clear with handle retirement/move source. */
    unsigned native_flags; /* Immutable owned-handle capabilities; known mask DVM_VFS_CANCEL_MASK. */
    int (*close_ready)(void *state); /* Optional1/0 close capability query under exclusive ownership; no allocation/I/O/VM access. */
} DVM_VfsHandle;


/* EN/RU: Adopt exactly one native file/directory into empty output; no allocation.
 * Empty inputs produce an empty handle. Invalid inputs preserve all ownership.
 * Принять один file/directory в пустой output; без allocation. Пустые inputs
 * дают пустой handle. Invalid inputs не меняют владение. Return one/zero. */
int dvm_vfs_adopt(DVM_VfsHandle *out, FVM_PortFile *file, FVM_PortDirectory *directory);


/* EN/RU: Move exclusive source into empty out, clearing source; one/zero.
 * No I/O or allocation. Перенести source в пустой out, очистить source; один/ноль. */
int dvm_vfs_move(DVM_VfsHandle *out, DVM_VfsHandle *source);


/* EN/RU: Clear owning handle before consuming native close; empty/NULL harmless.
 * All borrowers must have retired. Очистить handle до consuming close;
 * empty/NULL безопасны; заёмщики уже ушли. No managed callbacks. */
void dvm_vfs_close(DVM_VfsHandle *handle);


/* EN/RU: Check exclusively owned close capability without I/O or managed access.
 * Empty handles succeed; static CLOSE wins, otherwise query close_ready or return0.
 * Borrow handle/state through the query; no allocation, callback retention or ownership transfer.
 * Проверить exclusive close capability без I/O/VM; empty даёт1, иначе CLOSE либо
 * close_ready/0. Handle/state borrowed, без allocation/retention/передачи ownership. */
int dvm_vfs_close_cancellable(const DVM_VfsHandle *handle);


/* EN/RU: Borrow handle/arguments under caller's lifetime pin and operation FIFO.
 * output receives native file metadata.
 * output получает native metadata.
 * Dispatch info; absent operation returns UNSUPPORTED without touching outputs.
 * Заимствовать handle/arguments с lifetime pin/FIFO; dispatch info. Отсутствующая
 * операция возвращает UNSUPPORTED без изменения outputs. Native-only blocking. */
FVM_Result dvm_vfs_info(DVM_VfsHandle *handle, FVM_FileInfo *output);


/* EN/RU: Borrow handle/arguments under caller's lifetime pin and operation FIFO.
 * bytes has size bytes capacity; received acknowledges actual bytes.
 * bytes имеет ёмкость size; received сообщает actual bytes.
 * Dispatch read; absent operation returns UNSUPPORTED without touching outputs.
 * Заимствовать handle/arguments с lifetime pin/FIFO; dispatch read. Отсутствующая
 * операция возвращает UNSUPPORTED без изменения outputs. Native-only blocking. */
FVM_Result dvm_vfs_read(DVM_VfsHandle *handle, void *bytes, size_t size, size_t *received);


/* EN/RU: Borrow handle/arguments under caller's lifetime pin and operation FIFO.
 * bytes supplies size immutable bytes; return provider write status.
 * bytes задаёт size immutable bytes; вернуть write status provider.
 * Dispatch write; absent operation returns UNSUPPORTED without touching outputs.
 * Заимствовать handle/arguments с lifetime pin/FIFO; dispatch write. Отсутствующая
 * операция возвращает UNSUPPORTED без изменения outputs. Native-only blocking. */
FVM_Result dvm_vfs_write(DVM_VfsHandle *handle, const void *bytes, size_t size);


/* EN/RU: Borrow handle/arguments under caller's lifetime pin and operation FIFO.
 * offset/origin select position; position receives successful result.
 * offset/origin задают позицию; position получает успешный результат.
 * Dispatch seek; absent operation returns UNSUPPORTED without touching outputs.
 * Заимствовать handle/arguments с lifetime pin/FIFO; dispatch seek. Отсутствующая
 * операция возвращает UNSUPPORTED без изменения outputs. Native-only blocking. */
FVM_Result dvm_vfs_seek(DVM_VfsHandle *handle, long offset, int origin, long *position);


/* EN/RU: Borrow handle/arguments under caller's lifetime pin and operation FIFO.
 * entries/count receive an owned port batch, freed by entries_free.
 * entries/count получают свой port batch; освободить через entries_free.
 * Dispatch list; absent operation returns UNSUPPORTED without touching outputs.
 * Заимствовать handle/arguments с lifetime pin/FIFO; dispatch list. Отсутствующая
 * операция возвращает UNSUPPORTED без изменения outputs. Native-only blocking. */
FVM_Result dvm_vfs_list(DVM_VfsHandle *handle, FVM_FileInfo **entries, size_t *count);


/* EN/RU: Execute one chunk with precise callback or conservative legacy adapter.
 * Borrow handle/buffer under FD pin/FIFO, native-only blocking. Return confirmed
 * prefix plus cause; only actual failure triggers at most one info callback.
 * Выполнить один chunk с точным callback либо conservative legacy adapter.
 * Handle/buffer заимствованы под FD pin/FIFO. При отказе — не более одного info.
 * Success/short success/EOF never query metadata. No retry or layer switching. */
DVM_IoResult dvm_vfs_read_chunk(DVM_VfsHandle *handle, void *bytes, size_t size);


/* EN/RU: Same contract for immutable write bytes. Legacy failed writes have
 * unknown effects, never fabricated count=0 certainty. A positive request with
 * zero acknowledged bytes is NO_PROGRESS, not EOF. Zero-size success stays valid.
 * Тот же контракт для immutable write bytes. Legacy failure имеет unknown effects;
 * ноль при положительном request — NO_PROGRESS, не EOF. Zero-size success допустим. */
DVM_IoResult dvm_vfs_write_chunk(DVM_VfsHandle *handle, const void *bytes, size_t size);


/* EN/RU: Copy last chunk diagnostics while borrowing the same FD pin/FIFO.
 * One/zero for available/missing; no I/O and no metadata refresh.
 * Скопировать last diagnostics под тем же FD pin/FIFO; один/ноль, без I/O. */
int dvm_vfs_last_io(DVM_VfsHandle *handle, DVM_IoResult *output);


/* EN/RU: Legacy native adapters only; borrow matching state or return NULL.
 * No ownership transfer. Legacy adapters: заимствовать state либо NULL. */
FVM_PortFile *dvm_vfs_file(DVM_VfsHandle *handle);


/* EN/RU: Borrow matching native directory or return NULL; no ownership transfer.
 * Заимствовать native directory либо NULL; без передачи владения. */
FVM_PortDirectory *dvm_vfs_directory(DVM_VfsHandle *handle);

/* EN/RU: Invalidate list cursor after failed managed publication under the same
 * FD pin/FIFO. Native metadata only; no allocation, blocking or guest callbacks.
 * Пометить list cursor failed после ошибки managed publication с тем же FD
 * pin/FIFO; только native metadata, без allocation/blocking/guest callbacks. */
FVM_Result dvm_vfs_abort_list(DVM_VfsHandle *handle);
#endif
