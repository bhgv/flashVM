/* Verify actual OS-exclusive creation through the shared portable file API. */
#include "file_port.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


/* Task enumeration is unavailable in the test sandbox; ASan/UBSan remain active. */
int __lsan_is_turned_off(void)
{
    return 1;
}


/* Check one real contract and name failures; condition/message are borrowed. */
static int check(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL exclusive file: %s\n", message);
    }
    return condition;
}


/* Execute in a caller-owned private directory. Eight independent child opens
 * race for one name; exactly one may create/write it. No user files are touched. */
int main(void)
{
    FVM_PortFile *file = NULL;
    unsigned char bytes[256];
    unsigned char readback[256];
    size_t received = 0;
    int index;
    int status;
    int winners = 0;
    int barrier[2];
    char credit;
    pid_t children[8];
    for (index = 0; index < 256; ++index)
    {
        bytes[index] = (unsigned char)index;
    }
    if (!check(fvm_port_file_open("raw.bin", "wx", &file) == FVM_OK, "new wx") ||
        !check(fvm_port_file_write(file, bytes, sizeof(bytes)) == FVM_OK, "binary write"))
    {
        return 1;
    }
    fvm_port_file_close(file);
    if (!check(fvm_port_file_open("raw.bin", "wx", &file) != FVM_OK && file == NULL, "existing wx") ||
        !check(symlink("raw.bin", "linked.bin") == 0 && symlink("missing", "dangling.bin") == 0, "symlinks") ||
        !check(fvm_port_file_open("linked.bin", "wx", &file) != FVM_OK && file == NULL, "existing symlink") ||
        !check(fvm_port_file_open("dangling.bin", "wx", &file) != FVM_OK && file == NULL, "dangling symlink") ||
        !check(access("missing", F_OK) != 0, "dangling target untouched") ||
        !check(fvm_port_file_open("raw.bin", "r", &file) == FVM_OK, "reopen"))
    {
        return 1;
    }
    if (!check(fvm_port_file_read(file, readback, sizeof(readback), &received) == FVM_OK && received == sizeof(bytes) &&
                   !memcmp(bytes, readback, sizeof(bytes)),
               "existing bytes preserved"))
    {
        return 1;
    }
    fvm_port_file_close(file);
    if (!check(pipe(barrier) == 0, "start barrier"))
    {
        return 1;
    }
    for (index = 0; index < 8; ++index)
    {
        children[index] = fork();
        if (!check(children[index] >= 0, "fork"))
        {
            return 1;
        }
        if (children[index] == 0)
        {
            close(barrier[1]);
            if (read(barrier[0], &credit, 1) != 1)
            {
                _exit(4);
            }
            close(barrier[0]);
            if (fvm_port_file_open("race.bin", "wx", &file) != FVM_OK)
            {
                _exit(2);
            }
            status = fvm_port_file_write(file, bytes + index, 1) == FVM_OK;
            fvm_port_file_close(file);
            _exit(status ? 0 : 3);
        }
    }
    close(barrier[0]);
    if (!check(write(barrier[1], bytes, 8) == 8, "release creators"))
    {
        return 1;
    }
    close(barrier[1]);
    for (index = 0; index < 8; ++index)
    {
        if (!check(waitpid(children[index], &status, 0) == children[index] && WIFEXITED(status), "wait"))
        {
            return 1;
        }
        if (WEXITSTATUS(status) == 0)
        {
            ++winners;
        }
        else if (!check(WEXITSTATUS(status) == 2, "child write"))
        {
            return 1;
        }
    }
    if (!check(winners == 1, "one exclusive winner"))
    {
        return 1;
    }
    puts("PASS exclusive port: bytes=256 existing/symlinks=preserved race-winners=1");
    return 0;
}
