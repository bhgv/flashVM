/* Copyright (c) 2026 bhgv
 * EN: Check the C SDK mutation helpers against the same provider as DisVM.
 * RU: Проверить создание и удаление из SDK C с тем же провайдером, что у DisVM.
 */
#include "styxrpc/vfs_client.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN: Mount the passed socket, validate creation and remove results, release the session.
 * RU: Подключить переданный сокет, проверить создание и удаление, завершить сессию. */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 1;
    }
    StyxClient *session = styxclientmountfd(atoi(argv[1]), 8192, "", "", 3000, 4);
    StyxRpcVfsFile *file = NULL, *duplicate = NULL, *reader = NULL;
    size_t transferred = 0;
    char bytes[3];
    int good = session &&
        styxrpc_vfs_mkdir(session, "folder") == STYXRPC_IO_OK &&
        styxrpc_vfs_create(session, "folder/data", &file) == STYXRPC_IO_OK &&
        styxrpc_vfs_file_write(file, 0, "abc", 3, &transferred) == STYXRPC_IO_OK && transferred == 3 &&
        styxrpc_vfs_create(session, "folder/data", &duplicate) == STYXRPC_IO_FAILED &&
        styxrpc_vfs_remove(session, "folder") == STYXRPC_IO_FAILED &&
        styxrpc_vfs_create(session, "readonly/file", &duplicate) == STYXRPC_IO_FAILED &&
        styxrpc_vfs_open(session, "folder/data", STYXRPC_FILE_READ, &reader) == STYXRPC_IO_OK &&
        styxrpc_vfs_remove(session, "folder/data") == STYXRPC_IO_OK &&
        styxrpc_vfs_remove(session, "folder/data") == STYXRPC_IO_NOT_OPEN &&
        styxrpc_vfs_file_read(reader, 0, bytes, sizeof(bytes), &transferred) == STYXRPC_IO_OK &&
        transferred == 3 && !memcmp(bytes, "abc", 3) &&
        styxrpc_vfs_remove(session, "folder") == STYXRPC_IO_OK;
    styxrpc_vfs_close(duplicate, 0);
    styxrpc_vfs_close(reader, 0);
    styxrpc_vfs_close(file, 0);
    styxclientfree(session);
    puts(good ? "PASS C SDK mkdir, create and remove" : "FAIL C SDK mutations");
    return !good;
}
