/* Copyright (c) 2026 bhgv
 * EN: POSIX and Windows byte transfers on borrowed standard streams.
 * RU: Передача байтов через заимствованные стандартные потоки POSIX и Windows.
 */
#define FVM_STDIO_TRANSFER_MAX 65536UL /* Maximum native standard-stream fragment accepted per call. */


/* Read one bounded fragment from borrowed standard input.
 * descriptor must be zero; output receives the actual extent, including EOF zero. */
FVM_Result fvm_port_stdio_read(int descriptor, void *bytes, size_t capacity, size_t *received)
{
    long count;
    if (descriptor != 0 || !received || (!bytes && capacity))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *received = 0;
    if (!capacity)
    {
        return FVM_OK;
    }
    if (capacity > FVM_STDIO_TRANSFER_MAX)
    {
        capacity = FVM_STDIO_TRANSFER_MAX;
    }
    do
    {
#ifdef _WIN32
        count = _read(descriptor, bytes, (unsigned int)capacity);
#else
        count = (long)read(descriptor, bytes, capacity);
#endif
    } while (count < 0 && errno == EINTR);
    if (count < 0)
    {
        return FVM_ERROR;
    }
    *received = (size_t)count;
    return FVM_OK;
}


/* Write one bounded fragment to borrowed standard output/error.
 * descriptor is 1 or 2; written exposes partial writes so callers can retry the remainder. */
FVM_Result fvm_port_stdio_write(int descriptor, const void *bytes, size_t size, size_t *written)
{
    long count;
    if ((descriptor != 1 && descriptor != 2) || !written || (!bytes && size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    *written = 0;
    if (!size)
    {
        return FVM_OK;
    }
    if (fflush(descriptor == 1 ? stdout : stderr))
    {
        return FVM_ERROR;
    }
    if (size > FVM_STDIO_TRANSFER_MAX)
    {
        size = FVM_STDIO_TRANSFER_MAX;
    }
    do
    {
#ifdef _WIN32
        count = _write(descriptor, bytes, (unsigned int)size);
#else
        count = (long)write(descriptor, bytes, size);
#endif
    } while (count < 0 && errno == EINTR);
    if (count < 0)
    {
        return FVM_ERROR;
    }
    *written = (size_t)count;
    return FVM_OK;
}
