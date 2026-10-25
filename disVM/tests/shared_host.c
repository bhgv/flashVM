/* Exercise actual FlashVM ports: native threads/conditions and portable file I/O. */
#include <stdlib.h>
#include "disvm/flashvm_host.h"
#include "file_port.h"

#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <stdio.h>
#include <string.h>


/* Both native workers must enter before either may finish; a serial fiber
 * implementation cannot satisfy this barrier. Caller owns all shared state. */
typedef struct Barrier
{
    FVM_PortSync *sync;
    unsigned int arrived;
    unsigned int completed;
    int released;
} Barrier;


/* Block on the shared FlashVM condition until the owner releases both workers. */
static void native_worker(void *user)
{
    Barrier *barrier = (Barrier *)user;
    dvm_host_sync_lock(barrier->sync);
    barrier->arrived++;
    dvm_host_sync_wake_all(barrier->sync);
    while (!barrier->released)
    {
        dvm_host_sync_wait(barrier->sync);
    }
    barrier->completed++;
    dvm_host_sync_unlock(barrier->sync);
}


/* Validate real OS-thread rendezvous and binary file transfer on the selected port. */
int main(void)
{
    static const unsigned char bytes[] = {0xc0, 0x0c, 0x80, 0x30, 0, 255, 42};
    const char *path = "disvm-shared-host-test.bin";
    DVM_ModuleImage image = {0, 0, 0};
    FVM_PortFile *file = 0;
    FVM_PortThread *first;
    FVM_PortThread *second;
    Barrier barrier;
    memset(&barrier, 0, sizeof(barrier));
    barrier.sync = dvm_host_sync_create();
    if (!(barrier.sync))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    first = dvm_host_thread_start(native_worker, &barrier);
    second = dvm_host_thread_start(native_worker, &barrier);
    if (!(first && second))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_host_sync_lock(barrier.sync);
    while (barrier.arrived != 2)
    {
        dvm_host_sync_wait(barrier.sync);
    }
    if (!(barrier.completed == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    barrier.released = 1;
    dvm_host_sync_wake_all(barrier.sync);
    dvm_host_sync_unlock(barrier.sync);
    dvm_host_thread_join(first);
    dvm_host_thread_join(second);
    if (!(barrier.completed == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_host_sync_destroy(barrier.sync);

    if (!(fvm_port_file_open(path, "w", &file) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_port_file_write(file, bytes, sizeof(bytes)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_port_file_close(file);
    if (!(dvm_image_file("file:test", path, sizeof(bytes), 1024, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image.size == sizeof(bytes) && !memcmp(image.bytes, bytes, sizeof(bytes))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    if (!(dvm_image_file("file:truncated", path, sizeof(bytes) + 1, 1024, &image) == DVM_IMAGE_TRUNCATED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!image.bytes && !image.name))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(remove(path) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_file("file:missing", path, 4, 1024, &image) == DVM_IMAGE_IO))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
