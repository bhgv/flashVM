/* Exercise the C89 filesystem port independently of Dis/Flash heaps or callbacks. */
#include "filesystem_port.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Native address/UB instrumentation remains active without unavailable proc leak discovery. */
int __lsan_is_turned_off(void)
{
    return 1;
}


/* Report a failed external oracle condition without disabling calls under NDEBUG. */
static int require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL filesystem port: %s\n", message);
    }
    return condition;
}


/* Arguments supply private fixture directory, file, sparse file, moved name and
 * invalid-UTF directory. Own all opened handles/batches and close on failure.
 * Test setup uses stdio rename/remove only as an independent native identity oracle. */
int main(int argc, char **argv)
{
    FVM_PortFile *file = NULL;
    FVM_PortDirectory *directory = NULL;
    FVM_FileInfo metadata;
    FVM_FileInfo original;
    FVM_FileInfo sentinel;
    FVM_FileInfo *entries = NULL;
    size_t count = 0;
    size_t received;
    size_t total = 0;
    unsigned char byte;
    int ok = 0;
    if (argc != 6)
    {
        return 2;
    }
    if (!require(fvm_port_file_open("content://documents/not-a-path", "w", &file) == FVM_UNSUPPORTED && file == NULL,
                 "provider write cannot reach ordinary fopen"))
    {
        goto done;
    }
    memset(&metadata, 0x5a, sizeof(metadata));
    sentinel = metadata;
    if (!require(fvm_port_path_info("content://documents/not-a-path", &metadata) == FVM_UNSUPPORTED &&
                     memcmp(&metadata, &sentinel, sizeof(metadata)) == 0,
                 "URI rejection preserves output") ||
        !require(fvm_port_file_info(NULL, &metadata) == FVM_INVALID_ARGUMENT &&
                     memcmp(&metadata, &sentinel, sizeof(metadata)) == 0,
                 "nil file preserves output"))
    {
        goto done;
    }
    if (!require(fvm_port_path_info(argv[3], &metadata) == FVM_OK && metadata.length.high == 1 &&
                     metadata.length.low == 7,
                 "full 64-bit length") ||
        !require(fvm_port_file_open(argv[2], "r", &file) == FVM_OK, "open real file") ||
        !require(fvm_port_file_read(file, &byte, 1, &received) == FVM_OK && received == 1 && byte == 65,
                 "initial file byte") ||
        !require(fvm_port_file_info(file, &original) == FVM_OK && original.length.low == 4 &&
                     fvm_port_file_position(file) == 1,
                 "fstat preserves logical cursor") ||
        !require(fvm_port_file_read(file, &byte, 1, &received) == FVM_OK && received == 1 && byte == 0,
                 "binary NUL after metadata"))
    {
        goto done;
    }
#ifndef _WIN32
    if (!require(rename(argv[2], argv[4]) == 0, "native rename oracle") ||
        !require(fvm_port_file_info(file, &metadata) == FVM_OK &&
                     memcmp(&metadata.identity, &original.identity, sizeof(metadata.identity)) == 0,
                 "renamed open object identity") ||
        !require(remove(argv[4]) == 0 && fvm_port_file_info(file, &metadata) == FVM_OK && metadata.length.low == 4,
                 "unlinked open object metadata"))
    {
        goto done;
    }
#endif
    fvm_port_file_close(file);
    file = NULL;
    if (!require(fvm_port_directory_open(argv[1], &directory) == FVM_OK, "open directory"))
    {
        goto done;
    }
    for (;;)
    {
        size_t index;
        if (!require(fvm_port_directory_read(directory, &entries, &count) == FVM_OK && count <= FVM_FS_BATCH_MAX,
                     "bounded batch"))
        {
            goto done;
        }
        for (index = 0; index < count; index++)
        {
            if (!require(strcmp(entries[index].name, ".") && strcmp(entries[index].name, ".."), "dot filtering"))
            {
                goto done;
            }
        }
        total += count;
        fvm_port_directory_entries_free(entries);
        entries = NULL;
        if (!count)
        {
            break;
        }
    }
    if (!require(total >= 70 && fvm_port_directory_info(directory, &metadata) == FVM_OK && metadata.directory,
                 "all entries and descriptor metadata"))
    {
        goto done;
    }
    fvm_port_directory_abort(directory);
    count = 777;
    if (!require(fvm_port_directory_read(directory, &entries, &count) == FVM_ERROR && entries == NULL && count == 777,
                 "sticky publication failure preserves outputs"))
    {
        goto done;
    }
    fvm_port_directory_close(directory);
    directory = NULL;
#ifndef _WIN32
    if (!require(fvm_port_directory_open(argv[5], &directory) == FVM_OK, "open invalid-name fixture") ||
        !require(fvm_port_directory_read(directory, &entries, &count) == FVM_UNSUPPORTED && entries == NULL &&
                     count == 777,
                 "invalid UTF-8 is not silently replaced") ||
        !require(fvm_port_directory_read(directory, &entries, &count) == FVM_ERROR && entries == NULL && count == 777,
                 "invalid native batch cannot resume silently"))
    {
        goto done;
    }
#endif
    ok = 1;
done:
    fvm_port_directory_entries_free(entries);
    fvm_port_directory_close(directory);
    fvm_port_file_close(file);
    if (ok)
    {
        puts("PASS filesystem C89 port: exact metadata, open identity, cursors and failures");
    }
    return ok ? 0 : 1;
}
