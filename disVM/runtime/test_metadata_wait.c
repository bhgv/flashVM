/* Linux/glibc test-only barriers in actual stat64/fstat64/readdir64, never alternate VM code. */
#define _GNU_SOURCE
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int file_descriptor = -1;      /* Atomic descriptor captured from the private target fopen64. */
static int directory_descriptor = -1; /* Atomic descriptor captured after target opendir. */
static DIR *directory_stream;         /* Atomic target cursor identity, borrowed until close. */
static int blocked[4];                /* Atomic one-shot phase flags for path/file/directory/batch. */


/* Block the selected native phase in an inherited OS pipe read. kind is0..3;
 * no managed state is accessed. Diagnostics precede the wait; one permit releases
 * one phase. This controlled barrier is not an actual slow-filesystem benchmark. */
static void barrier(int kind)
{
    static const char *markers[] = {"METADATA_BLOCKED path\n", "METADATA_BLOCKED file\n", "METADATA_BLOCKED directory\n", "METADATA_BLOCKED batch\n"};
    const char *selection = getenv("DVM_TEST_METADATA_BARRIER");
    char permit;
    ssize_t received;
    size_t length;
    if (!selection || !__sync_bool_compare_and_swap(&blocked[kind], 0, 1))
    {
        return;
    }
    length = strlen(markers[kind]);
    if (write(2, markers[kind], length) != (ssize_t)length)
    {
        _exit(91);
    }
    do
    {
        received = read(atoi(selection), &permit, 1);
    }
    while (received < 0 && errno == EINTR);
    if (received != 1 || permit != '1')
    {
        _exit(92);
    }
}


/* Forward fopen64 and remember only the ordinary file used by this fixture.
 * path/mode follow libc ownership; no stream is retained or closed here. */
FILE *fopen64(const char *path, const char *mode)
{
    FILE *(*open_file)(const char *, const char *);
    void *symbol = dlsym(RTLD_NEXT, "fopen64");
    FILE *stream;
    if (!symbol || sizeof(open_file) != sizeof(symbol))
    {
        return NULL;
    }
    memcpy(&open_file, &symbol, sizeof(open_file));
    stream = open_file(path, mode);
    if (stream && strcmp(path, "metadata-fd.bin") == 0)
    {
        __sync_lock_test_and_set(&file_descriptor, fileno(stream));
    }
    return stream;
}


/* Forward opendir; record the fixture cursor only AFTER successful construction
 * so internal libc initialization cannot trigger an early metadata barrier. */
DIR *opendir(const char *path)
{
    DIR *(*open_directory)(const char *);
    void *symbol = dlsym(RTLD_NEXT, "opendir");
    DIR *directory;
    if (!symbol || sizeof(open_directory) != sizeof(symbol))
    {
        return NULL;
    }
    memcpy(&open_directory, &symbol, sizeof(open_directory));
    directory = open_directory(path);
    if (directory && strcmp(path, "metadata-dir") == 0)
    {
        __sync_lock_test_and_set(&directory_descriptor, dirfd(directory));
        __atomic_store_n(&directory_stream, directory, __ATOMIC_SEQ_CST);
    }
    return directory;
}


/* Forward the real large-file stat. Only the exact UTF-8 fixture path pauses;
 * other paths/loader queries remain genuine and permit concurrent conversions. */
int stat64(const char *path, struct stat64 *output)
{
    int (*query)(const char *, struct stat64 *);
    void *symbol = dlsym(RTLD_NEXT, "stat64");
    if (!symbol || sizeof(query) != sizeof(symbol))
    {
        return -1;
    }
    memcpy(&query, &symbol, sizeof(query));
    if (strcmp(path, "metadata-path-Я.bin") == 0)
    {
        barrier(0);
    }
    return query(path, output);
}


/* Forward the genuine descriptor query after its selected native barrier.
 * output remains libc-owned until return; descriptors are read atomically. */
int fstat64(int descriptor, struct stat64 *output)
{
    int (*query)(int, struct stat64 *);
    void *symbol = dlsym(RTLD_NEXT, "fstat64");
    if (!symbol || sizeof(query) != sizeof(symbol))
    {
        return -1;
    }
    memcpy(&query, &symbol, sizeof(query));
    if (descriptor == __sync_add_and_fetch(&file_descriptor, 0))
    {
        barrier(1);
    }
    else if (descriptor == __sync_add_and_fetch(&directory_descriptor, 0))
    {
        barrier(2);
    }
    return query(descriptor, output);
}


/* Forward the actual large-file directory read; block only the first entry call
 * on the fixture cursor, after its metadata phases have been acknowledged. */
struct dirent64 *readdir64(DIR *directory)
{
    struct dirent64 *(*read_directory)(DIR *);
    void *symbol = dlsym(RTLD_NEXT, "readdir64");
    if (!symbol || sizeof(read_directory) != sizeof(symbol))
    {
        return NULL;
    }
    memcpy(&read_directory, &symbol, sizeof(read_directory));
    if (directory == __atomic_load_n(&directory_stream, __ATOMIC_SEQ_CST))
    {
        barrier(3);
    }
    return read_directory(directory);
}
