/* Copyright (c) 2026 bhgv
 * EN: Check the live VFS aliases of redirected standard descriptors.
 * RU: Проверить виртуальные псевдонимы стандартных дескрипторов.
 */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "launch_files.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


/* EN: Check an inherited regular stdin shares its position and stays read-only.
 * RU: Проверить общую позицию обычного stdin и запрет записи. */
int scalar_verify_host_stdin(void)
{
    int valid;
    DVM_Provider *directory = dvm_launch_fd_view(NULL, &valid);
    DVM_VfsHandle reader = {0}, writer = {0};
    valid = valid && directory &&
        dvm_provider_open(directory, "0", "r", &reader) == FVM_OK &&
        dvm_provider_open(directory, "0", "wo", &writer) != FVM_OK;
    dvm_provider_release(directory);
    if (valid)
    {
        char first[4], last[4];
        DVM_IoResult result = dvm_vfs_read_chunk(&reader, first, sizeof(first));
        valid = result.status == FVM_OK && result.count == sizeof(first) &&
            !memcmp(first, "left", sizeof(first)) &&
            read(STDIN_FILENO, last, sizeof(last)) == sizeof(last) &&
            !memcmp(last, "next", sizeof(last));
    }
    dvm_vfs_close(&reader);
    dvm_vfs_close(&writer);
    return valid;
}


/* EN: Verify socket stdio can transfer both ways without changing its native flags.
 * RU: Проверить двусторонний обмен через сокет без изменения флагов FD. */
int scalar_verify_host_socket(void)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    int valid;
    DVM_Provider *directory = dvm_launch_fd_view(NULL, &valid);
    DVM_VfsHandle stream = {0};
    valid = flags >= 0 && !(flags & O_NONBLOCK) && valid && directory &&
        dvm_provider_open(directory, "0", "rw", &stream) == FVM_OK;
    dvm_provider_release(directory);
    if (valid)
    {
        char bytes[4];
        DVM_IoResult read = dvm_vfs_read_chunk(&stream, bytes, sizeof(bytes));
        DVM_IoResult empty = dvm_vfs_read_chunk(&stream, bytes + 3, 1);
        DVM_IoResult write = dvm_vfs_write_chunk(&stream, "next", 4);
        valid = read.status == FVM_OK && read.count == 4 && !memcmp(bytes, "left", 4) &&
            empty.stop == DVM_IO_AGAIN && empty.count == 0 &&
            write.status == FVM_OK && write.count == 4 && fcntl(STDIN_FILENO, F_GETFL) == flags;
    }
    dvm_vfs_close(&stream);
    return valid;
}
