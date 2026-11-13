/* Copyright (c) 2026 bhgv
 * EN: Cancellable standard socket IO without changing shared descriptor flags.
 * RU: Отменяемый standard socket IO без изменения общих descriptor flags.
 */
#include <sys/socket.h>


/* [GC:] EN: Transfer native staging bytes through a borrowed connected socket.
 * wait/ticket keep the original resident cancellable; no managed access or flags
 * mutation occurs outside its heap lease. Poll never owns the queue mutex;
 * only a bounded MSG_DONTWAIT syscall is serialized with cancellation. Preserve
 * committed write prefixes; EOF is a successful zero-byte read. timeout is the
 * existing admission policy, not an independently renewed network deadline.
 * RU: Передать native staging через borrowed connected socket. wait/ticket
 * сохраняют отменяемость исходного resident; managed access и смены fd flags
 * вне heap lease нет. Poll не держит очередь; с отменой сериализуется только
 * bounded MSG_DONTWAIT syscall. Сохранить write prefix; EOF — успешный read
 * нуля bytes. timeout задаёт admission policy, не новый network deadline. */
static FVM_Result transfer_stdio_socket(NativeGuestWait *wait,
                                        int wake_fd,
                                        int descriptor,
                                        void *input,
                                        const void *output,
                                        size_t capacity,
                                        size_t *transferred,
                                        int timeout)
{
    int writing = input == NULL;
    while (wait_stdio_pipe(wait, wake_fd, descriptor, writing ? POLLOUT : POLLIN, timeout))
    {
        ssize_t count;
        int failure;
        fvm_port_sync_lock(channel_queue_mutex);
        if (wait->cancelled)
        {
            fvm_port_sync_unlock(channel_queue_mutex);
            return *transferred ? FVM_OK : FVM_ERROR;
        }
        if (writing)
        {
            count = send(descriptor,
                         (const unsigned char *)output + *transferred,
                         capacity - *transferred,
                         MSG_DONTWAIT | MSG_NOSIGNAL);
        }
        else
        {
            count = recv(descriptor, input, capacity, MSG_DONTWAIT);
        }
        failure = errno;
        if (count >= 0 && !writing)
        {
            wait->wake_fd = -1;
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        if (count < 0 && retry_stdio_splice(failure, timeout))
        {
            continue;
        }
        if (count < 0 || (!count && writing))
        {
            return *transferred ? FVM_OK : FVM_ERROR;
        }
        *transferred += (size_t)count;
        if (!writing || *transferred == capacity)
        {
            return FVM_OK;
        }
    }
    return *transferred ? FVM_OK : FVM_ERROR;
}
