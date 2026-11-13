/* Copyright (c) 2026 bhgv
 * EN: Bounded socket byte transport with a single absolute monotonic deadline.
 * RU: Ограниченный socket transport с одним абсолютным monotonic deadline.
 */
#ifndef DVM_REMOTE_STREAM_H
#define DVM_REMOTE_STREAM_H
#include <stddef.h>
#include <stdint.h>

#define DVM_REMOTE_IO_LIMIT 65536U /* Maximum bytes per exact transport operation. */


/* EN/RU: Transport outcome, independent of RPC admission/effect completion.
 * Transport outcome не сообщает об RPC admission или завершении эффекта. */
typedef enum
{
    DVM_REMOTE_IO_OK,      /* All requested bytes transferred. */
    DVM_REMOTE_IO_EOF,     /* Peer ended its byte stream; transferred may be nonzero. */
    DVM_REMOTE_IO_TIMEOUT, /* Shared deadline expired, possibly after a partial transfer. */
    DVM_REMOTE_IO_ERROR,   /* Native errno is preserved in the result. */
    DVM_REMOTE_IO_INVALID  /* Invalid descriptor/buffer/size; no transfer. */
} DvmRemoteIoStatus;


/* EN/RU: Preserve partial progress so callers never infer cancellation or retry.
 * Сохранить частичный прогресс, чтобы caller не выводил отмену или replay. */
typedef struct
{
    DvmRemoteIoStatus status; /* Terminal result of this byte operation only. */
    size_t transferred;       /* Exact bytes consumed from or committed to socket. */
    int system_error;         /* errno for IO_ERROR; zero for other outcomes. */
} DvmRemoteIoResult;


/* EN: Read monotonic milliseconds into output, return zero or native errno.
 * RU: Записать monotonic milliseconds в output; вернуть ноль либо native errno.
 * Thread safe; no allocation or managed access; NULL output returns EINVAL. */
int dvm_remote_now_ms(uint64_t *output);


/* EN: Borrow an already connected socket and native buffer through completion;
 * capacity <= IO_LIMIT, deadline is absolute CLOCK_MONOTONIC milliseconds.
 * No allocation, descriptor flag changes, auto-replay or managed access. A single
 * read and a single write may run concurrently; callers serialize same-direction
 * operations and keep fd alive (shutdown may cancel; close/reuse must wait).
 * RU: Заимствовать connected socket и native buffer до завершения; capacity <=
 * IO_LIMIT, deadline — абсолютные CLOCK_MONOTONIC milliseconds. Нет allocation,
 * смены fd flags, replay или managed access. Один read и write могут идти вместе;
 * однонаправленные calls сериализует caller. fd жив до возврата; shutdown допустим,
 * close/reuse ждёт возврата. Частичный outcome не означает отмену owner effect. */
DvmRemoteIoResult dvm_remote_read_exact(int fd, void *buffer, size_t capacity, uint64_t deadline);
DvmRemoteIoResult dvm_remote_write_exact(int fd, const void *buffer, size_t capacity, uint64_t deadline);
#endif
