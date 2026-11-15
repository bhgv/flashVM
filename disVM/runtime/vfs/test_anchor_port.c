/* Copyright (c) 2026 bhgv
 * EN: Real anchor rename, independent cursors, concurrent opens and late stdio failure.
 * RU: Настоящий rename anchor, независимые cursors, concurrent opens и late stdio failure.
 */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* POSIX symlink fixture declarations in strict C99 builds. */
#endif
#include "anchor_port.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define ANCHOR_TEST_PATH 4096                             /* Native fixture path capacity including terminator. */
#define ANCHOR_TEST_OWNERS 8                              /* Concurrent independently positioned file clients. */
#define ANCHOR_TEST_ROUNDS 128                            /* Successful child opens per OS owner. */
static int fail_stdio;                                    /* Main-thread-only injection, before test owners start. */
static int failed_descriptor = -1;                        /* Actual FD passed to the omitted fdopen. */
static unsigned removers;                                 /* Successful unlink owners, protected by test mutex. */
static unsigned winners;                                  /* Successful O_EXCL owners, under mutex. */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Test result only. */
FILE *__real_fdopen(int, const char *);


/* EN/RU: Omit one libc allocation boundary, returning ENOMEM without closing FD.
 * Убрать одну libc allocation boundary, вернуть ENOMEM без закрытия FD. */
FILE *__wrap_fdopen(int descriptor, const char *mode)
{
    if (fail_stdio)
    {
        fail_stdio = 0;
        failed_descriptor = descriptor;
        errno = ENOMEM;
        return NULL;
    }
    return __real_fdopen(descriptor, mode);
}


/* EN/RU: Require condition or exit with message; no implicit assertion build mode.
 * Проверить condition либо exit с message; без режима неявных assertions. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "anchor port: %s\n", message);
        exit(2);
    }
}


/* EN/RU: Write exact text to borrowed native path using the ordinary port.
 * Записать точный text по native path через обычный port; закрыть свой file. */
static void write_path(const char *path, const char *text)
{
    FVM_PortFile *file = NULL;
    require(fvm_port_file_open(path, "w", &file) == FVM_OK, "fixture open");
    require(fvm_port_file_write(file, text, strlen(text)) == FVM_OK, "fixture write");
    fvm_port_file_close(file);
}


/* EN/RU: Read ABC in two independent fragments from an anchor; release own file.
 * Прочитать ABC двумя fragments от anchor; освободить свой file. */
static void read_token(FVM_PortAnchor *anchor)
{
    FVM_PortFile *file = NULL;
    char bytes[3];
    size_t received;
    require(fvm_port_file_open_at(anchor, "token", "r", &file) == FVM_OK, "relative read open");
    require(fvm_port_file_read(file, bytes, 1, &received) == FVM_OK && received == 1 && bytes[0] == 'A',
            "first fragment");
    require(fvm_port_file_read(file, bytes + 1, 2, &received) == FVM_OK && received == 2 && !memcmp(bytes, "ABC", 3),
            "shared native cursor corrupted fragment");
    fvm_port_file_close(file);
}


/* EN/RU: Require token in an independent cursor batch; batch/cursor are consumed.
 * Требовать token в независимом cursor batch; поглотить batch/cursor. */
static void directory_token(FVM_PortDirectory *directory)
{
    FVM_FileInfo *entries = NULL;
    size_t count, index;
    int found = 0;
    require(fvm_port_directory_read(directory, &entries, &count) == FVM_OK, "relative directory read");
    for (index = 0; index < count; index++)
    {
        if (!strcmp(entries[index].name, "token"))
        {
            found = 1;
        }
    }
    require(found, "independent directory offset lost");
    fvm_port_directory_entries_free(entries);
    fvm_port_directory_close(directory);
}


/* EN/RU: Race exclusive creation once, then independently read held directory.
 * Один раз конкурировать за exclusive create, затем читать held directory. */
static void *owner(void *argument)
{
    FVM_PortAnchor *anchor = argument;
    FVM_PortFile *file = NULL;
    unsigned index;
    if (fvm_port_file_open_at(anchor, "exclusive", "wx", &file) == FVM_OK)
    {
        pthread_mutex_lock(&mutex);
        winners++;
        pthread_mutex_unlock(&mutex);
        fvm_port_file_close(file);
    }
    else
    {
        require(!file, "failed exclusive open published a handle");
    }
    if (fvm_port_anchor_remove_at(anchor, "one-remove") == FVM_OK)
    {
        pthread_mutex_lock(&mutex);
        removers++;
        pthread_mutex_unlock(&mutex);
    }
    for (index = 0; index < ANCHOR_TEST_ROUNDS; index++)
    {
        read_token(anchor);
    }
    return NULL;
}


/* EN: Run in an empty borrowed directory; own all handles until threads join.
 * Real rename installs a decoy at the old name. Return zero only after complete
 * cursor, create, metadata and injected-allocation checks succeed.
 * RU: Запускать в пустом каталоге; держать handles до join. Настоящий rename
 * устанавливает decoy по старому имени. Ноль после cursor/create/metadata и OOM checks. */
int main(void)
{
    FVM_PortAnchor *anchor = NULL, *child = NULL;
    FVM_PortDirectory *first = NULL, *second = NULL;
    FVM_PortFile *file = NULL;
    FVM_FileInfo before, after;
    pthread_t owners[ANCHOR_TEST_OWNERS];
    unsigned index;
    char cwd[ANCHOR_TEST_PATH];
    require(getcwd(cwd, sizeof(cwd)) != NULL && cwd[0] == '/', "absolute cwd");
    require(fvm_port_anchor_open("/", &anchor) == FVM_OK, "startup root anchor");
    require(fvm_port_anchor_open_at(anchor, cwd[1] ? cwd + 1 : ".", &child) == FVM_OK,
            "startup cwd relative to root");
    fvm_port_anchor_close(child);
    fvm_port_anchor_close(anchor);
    child = NULL;
    anchor = NULL;
    require(!mkdir("original", 0700), "make original");
    write_path("original/token", "ABC");
    require(fvm_port_file_open("original/token", "wo", &file) == FVM_OK, "existing write-only path");
    require(fvm_port_file_write(file, "A", 1) == FVM_OK, "nonappend direct write");
    fvm_port_file_close(file);
    file = NULL;
    require(fvm_port_anchor_open("original", &anchor) == FVM_OK, "anchor open");
    require(fvm_port_anchor_info_at(anchor, ".", &before) == FVM_OK, "original identity");
    require(fvm_port_directory_open_at(anchor, ".", &first) == FVM_OK &&
                fvm_port_directory_open_at(anchor, ".", &second) == FVM_OK,
            "two independent directory cursors");
    require(!rename("original", "moved") && !mkdir("original", 0700), "real directory rename");
    write_path("original/token", "ZZZ");
    require(fvm_port_anchor_info_at(anchor, ".", &after) == FVM_OK && before.identity.high == after.identity.high &&
                before.identity.low == after.identity.low,
            "directory identity changed");
    require(fvm_port_anchor_access_at(anchor, ".", FVM_ACCESS_WRITE | FVM_ACCESS_EXEC) == FVM_OK,
            "effective directory write/search");
    require(fvm_port_anchor_access_at(anchor, "token", FVM_ACCESS_READ) == FVM_OK &&
                fvm_port_anchor_access_at(anchor, "token", FVM_ACCESS_EXEC) != FVM_OK,
            "effective file read/execute");
    require(fvm_port_anchor_access_at(anchor, "/token", FVM_ACCESS_READ) == FVM_INVALID_ARGUMENT &&
                fvm_port_anchor_access_at(anchor, "token", FVM_ACCESS_MASK << 1) == FVM_INVALID_ARGUMENT,
            "invalid effective access request accepted");
    read_token(anchor);
    directory_token(first);
    directory_token(second);
    require(fvm_port_anchor_open_at(anchor, ".", &child) == FVM_OK, "child anchor");
    require(fvm_port_file_open_at(child, "/token", "r", &file) == FVM_INVALID_ARGUMENT && !file,
            "absolute child accepted");
    write_path("moved/remove", "ABC");
    write_path("original/remove", "ZZZ");
    require(fvm_port_anchor_remove_at(child, "remove") == FVM_OK, "anchored remove");
    require(fvm_port_path_info("original/remove", &after) == FVM_OK &&
                fvm_port_path_info("moved/remove", &after) != FVM_OK,
            "remove redirected to decoy");
    write_path("outside", "ABC");
    require(!mkdir("outside-dir", 0700) && !symlink("../outside", "moved/link") &&
                !symlink("../outside-dir", "moved/dir-link") && !symlink("../absent", "moved/dangling"),
            "symlink fixture");
    require(fvm_port_anchor_remove_at(child, "link") == FVM_OK &&
                fvm_port_anchor_remove_at(child, "dir-link") == FVM_OK &&
                fvm_port_anchor_remove_at(child, "dangling") == FVM_OK &&
                fvm_port_path_info("outside", &after) == FVM_OK && fvm_port_path_info("outside-dir", &after) == FVM_OK,
            "remove followed final symlink");
    require(!mkdir("moved/empty", 0700) && !mkdir("moved/full", 0700), "directory remove fixtures");
    write_path("moved/full/keep", "ABC");
    require(fvm_port_anchor_remove_at(child, "empty") == FVM_OK && fvm_port_anchor_remove_at(child, "full") != FVM_OK &&
                fvm_port_path_info("moved/full/keep", &after) == FVM_OK,
            "recursive removal occurred");
    require(fvm_port_anchor_remove_at(child, ".") == FVM_INVALID_ARGUMENT &&
                fvm_port_anchor_remove_at(child, "..") == FVM_INVALID_ARGUMENT &&
                fvm_port_anchor_remove_at(child, "/outside") == FVM_INVALID_ARGUMENT,
            "invalid removal accepted");
    write_path("moved/duplex", "ABCDE");
    require(fvm_port_file_open_at(child, "duplex", "rwo", &file) == FVM_OK, "existing duplex open");
    {
        char bytes[5];
        size_t count;
        int available;
        long position;
        require(fvm_port_file_read(file, bytes, 2, &count) == FVM_OK && count == 2 && !memcmp(bytes, "AB", 2),
                "duplex initial shared position");
        require(fvm_port_file_write(file, "X", 1) == FVM_OK &&
                fvm_port_file_read(file, bytes, 1, &count) == FVM_OK && count == 1 && bytes[0] == 'D',
                "duplex overwrite or read cursor");
        require(fvm_port_file_available(file, &available) == FVM_OK && available &&
                fvm_port_file_write(file, "Y", 1) == FVM_OK, "duplex peek changed write cursor");
        require(fvm_port_file_seek(file, 0, FVM_FILE_SEEK_START, &position) == FVM_OK &&
                fvm_port_file_read(file, bytes, sizeof(bytes), &count) == FVM_OK && count == sizeof(bytes) &&
                !memcmp(bytes, "ABXDY", sizeof(bytes)), "duplex truncated/appended or seek diverged");
    }
    fvm_port_file_close(file);
    file = NULL;
    require(fvm_port_file_open_at(child, "missing-rwo", "rwo", &file) != FVM_OK && !file &&
            fvm_port_path_info("moved/missing-rwo", &after) != FVM_OK, "duplex created missing file");
    write_path("moved/one-remove", "ABC");
    fail_stdio = 1;
    require(fvm_port_file_open_at(child, "token", "w", &file) != FVM_OK && !file, "stdio failure not injected");
    require(failed_descriptor >= 0 && fcntl(failed_descriptor, F_GETFD) < 0 && errno == EBADF,
            "late failure leaked real descriptor");
    require(fvm_port_file_open_at(child, "absent", "wo", &file) != FVM_OK && !file,
            "write-only created missing entry");
    fail_stdio = 1;
    require(fvm_port_file_open_at(child, "token", "wo", &file) != FVM_OK && !file,
            "write-only stdio failure not injected");
    require(failed_descriptor >= 0 && fcntl(failed_descriptor, F_GETFD) < 0 && errno == EBADF,
            "write-only late failure leaked descriptor");
    read_token(child);
    for (index = 0; index < ANCHOR_TEST_OWNERS; index++)
    {
        require(!pthread_create(&owners[index], NULL, owner, child), "start native owner");
    }
    for (index = 0; index < ANCHOR_TEST_OWNERS; index++)
    {
        require(!pthread_join(owners[index], NULL), "join native owner");
    }
    require(removers == 1, "concurrent unlink did not have one winner");
    require(winners == 1, "exclusive creation did not have one owner");
    require(fvm_port_file_open_at(child, "token", "r", &file) == FVM_OK, "retained removal FD");
    require(fvm_port_anchor_remove_at(child, "token") == FVM_OK, "remove opened file");
    {
        char payload[3];
        size_t count;
        require(fvm_port_file_read(file, payload, sizeof(payload), &count) == FVM_OK && count == sizeof(payload) &&
                    !memcmp(payload, "ABC", sizeof(payload)),
                "unlink invalidated open descriptor");
    }
    fvm_port_file_close(file);
    fvm_port_anchor_close(child);
    fvm_port_anchor_close(anchor);
    puts(
        "PASS anchor rename, cursors, eight owners, create/remove races, symlinks, open-FD unlink and late stdio failure");
    return 0;
}
