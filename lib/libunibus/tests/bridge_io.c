/* Copyright (c) 2026 bhgv
 * EN: Deterministic short-I/O/EINTR/EOF tests of the actual gateway transfer
 * helper and terminal-output behavior. Syscall wrappers never touch real fd 4242.
 * RU: Детерминированные short-I/O/EINTR/EOF тесты настоящего helper шлюза и
 * отказа вывода. Syscall wrappers не используют реальный fd 4242.
 */
#include "bridge.h"
#include "flashvm/srecord.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FAKE_FD 4242 /* Private wrapper selector, not an opened process descriptor. */
#define CAPACITY 4096U /* Bounded observed bytes; exceeding this fails the test explicitly. */


/* EN/RU: Ordered syscall scripts are test inputs, not a second implementation of bridge_exact.
 * Порядок syscall — вход теста, не вторая реализация bridge_exact.
 */
enum Scenario
{
    READ_PROGRESS,       /* One byte, interrupted read, then the remaining span. */
    READ_EOF,            /* Two bytes, then EOF without completing the requested span. */
    WRITE_PROGRESS,      /* Two bytes, interrupted write, then the remaining span. */
    WRITE_TIMEOUT,       /* Backend EAGAIN after partial progress is a socket timeout, not output backpressure. */
    WRITE_ZERO,          /* A zero write must terminate, not spin. */
    WRITE_PARTIAL_ERROR, /* Two bytes accepted, then EPIPE: accepted bytes are not undone. */
    OUTPUT_GOOD,         /* Capture checked records through two-byte physical writes. */
    OUTPUT_BAD           /* Partial carrier write followed by EPIPE; the direction must latch dead. */
};

static enum Scenario scenario;       /* Current serial syscall script. */
static unsigned calls;               /* Number of calls made, including interrupted/failed calls. */
static size_t transferred;           /* Bytes physically accepted by the script. */
static unsigned char *read_base;     /* Expected caller destination start for fake reads. */
static const unsigned char text[] = "abcde"; /* Fixed five-byte source; NUL is not part of the transfer. */
static unsigned char output[CAPACITY]; /* Captured physical stdout, never mixed with diagnostics. */
static int invalid_cursor;           /* Any replay, skipped byte or wrong remaining extent fails the case. */

extern ssize_t __real_read(int, void *, size_t);
extern ssize_t __real_write(int, const void *, size_t);


/* EN: For fd=FAKE_FD, consume the current read script into borrowed bytes/size.
 * Other descriptors forward to the real OS. Return real-style counts/-1 and errno.
 * RU: Для fd=FAKE_FD выполнить read script в bytes/size. Остальные fd — настоящий OS.
 * Вернуть обычные counts/-1 с errno; владение буферами не передаётся.
 */
ssize_t __wrap_read(int fd, void *bytes, size_t size)
{
    if (fd != FAKE_FD)
    {
        return __real_read(fd, bytes, size);
    }
    calls++;
    if ((unsigned char *)bytes != read_base + transferred || size != 5 - transferred)
    {
        invalid_cursor = 1;
        errno = EINVAL;
        return -1;
    }
    if (scenario == READ_PROGRESS && calls == 2)
    {
        errno = EINTR;
        return -1;
    }
    if (scenario == READ_EOF && calls == 2)
    {
        return 0;
    }
    size_t amount = calls == 1 ? (scenario == READ_EOF ? 2 : 1) : size;
    memcpy(bytes, text + transferred, amount);
    transferred += amount;
    return (ssize_t)amount;
}


/* EN: Simulate writes for FAKE_FD or stdout under OUTPUT_* scenarios; all other
 * calls use the real OS. bytes/size must describe only the remaining span.
 * RU: Моделировать запись FAKE_FD либо stdout при OUTPUT_*. Остальные вызовы — OS.
 * bytes/size должны описывать только ещё не принятый диапазон.
 */
ssize_t __wrap_write(int fd, const void *bytes, size_t size)
{
    if (fd != FAKE_FD && !(fd == STDOUT_FILENO && scenario >= OUTPUT_GOOD))
    {
        return __real_write(fd, bytes, size);
    }
    calls++;
    if (fd == FAKE_FD && ((const unsigned char *)bytes != text + transferred || size != 5 - transferred))
    {
        invalid_cursor = 1;
        errno = EINVAL;
        return -1;
    }
    if (scenario == WRITE_ZERO)
    {
        return 0;
    }
    if (scenario == WRITE_PROGRESS && calls == 2)
    {
        errno = EINTR;
        return -1;
    }
    if ((scenario == WRITE_PARTIAL_ERROR || scenario == WRITE_TIMEOUT || scenario == OUTPUT_BAD) && calls >= 2)
    {
        errno = scenario == WRITE_TIMEOUT ? EAGAIN : EPIPE;
        return -1;
    }
    if (scenario == OUTPUT_GOOD && calls == 1)
    {
        errno = EAGAIN;
        return -1;
    }
    size_t amount = scenario == WRITE_PROGRESS && calls >= 3 ? size : (size > 2 ? 2 : size);
    if (amount > CAPACITY - transferred)
    {
        invalid_cursor = 1;
        errno = ENOSPC;
        return -1;
    }
    memcpy(output + transferred, bytes, amount);
    transferred += amount;
    return (ssize_t)amount;
}


/* EN: Select the next scenario and clear observations, not the library's output state.
 * RU: Выбрать next и очистить наблюдения; состояние вывода самой библиотеки не сбрасывается.
 */
static void select_case(enum Scenario next)
{
    scenario = next;
    calls = 0;
    transferred = 0;
    invalid_cursor = 0;
}


/* EN: No arguments. Run real transfer helpers against prescribed partial syscalls.
 * Return 1 for replay/wrong cursor/false success or output reuse after failure, else 0.
 * RU: Аргументов нет. Проверить настоящие helpers заданными partial syscalls.
 * Вернуть 1 при replay/неверном cursor/ложном успехе/повторе отказавшего вывода, иначе 0.
 */
int main(void)
{
    unsigned char received[5] = {0};
    read_base = received;
    select_case(READ_PROGRESS);
    if (!bridge_exact(FAKE_FD, received, 5, 0) || transferred != 5 || calls != 3 || invalid_cursor || memcmp(received, text, 5))
    {
        fprintf(stderr, "read failed to retain partial progress across EINTR\n");
        return 1;
    }
    select_case(READ_EOF);
    if (bridge_exact(FAKE_FD, received, 5, 0) || transferred != 2 || calls != 2 || invalid_cursor)
    {
        fprintf(stderr, "partial EOF was reported as a complete read\n");
        return 1;
    }
    select_case(WRITE_PROGRESS);
    if (!bridge_exact(FAKE_FD, (void *)text, 5, 1) || transferred != 5 || calls != 3 || invalid_cursor || memcmp(output, text, 5))
    {
        fprintf(stderr, "write replayed or lost bytes after short write/EINTR\n");
        return 1;
    }
    select_case(WRITE_ZERO);
    if (bridge_exact(FAKE_FD, (void *)text, 5, 1) || calls != 1 || transferred || invalid_cursor)
    {
        fprintf(stderr, "zero write was not terminal no-progress\n");
        return 1;
    }
    select_case(WRITE_PARTIAL_ERROR);
    if (bridge_exact(FAKE_FD, (void *)text, 5, 1) || transferred != 2 || calls != 2 || invalid_cursor)
    {
        fprintf(stderr, "failed write concealed partial physical effects\n");
        return 1;
    }
    select_case(WRITE_TIMEOUT);
    if (bridge_exact(FAKE_FD, (void *)text, 5, 1) || transferred != 2 || calls != 2 || invalid_cursor)
    {
        fprintf(stderr, "backend timeout was confused with physical output backpressure\n");
        return 1;
    }
    select_case(OUTPUT_GOOD);
    if (!bridge_reply(7, 1, text, 5))
    {
        return 1;
    }
    FVM_SRecord outer;
    FVM_SRecord inner;
    size_t used;
    if (fvm_srecord_read(output, transferred, CAPACITY, &outer, &used) != FVM_S_OK || used != transferred || outer.tag != 7 ||
        fvm_srecord_read(outer.payload, outer.size, CAPACITY, &inner, &used) != FVM_S_OK || used != outer.size ||
        inner.tag != 1 || inner.size != 5 || memcmp(inner.payload, text, 5))
    {
        fprintf(stderr, "fragmented physical output changed nested record bytes\n");
        return 1;
    }
    select_case(OUTPUT_BAD);
    if (bridge_record(7, text, 5) || transferred != 2 || calls != 2)
    {
        return 1;
    }
    if (bridge_record(8, text, 5) || calls != 2 || transferred != 2)
    {
        fprintf(stderr, "gateway reused a physical output direction after partial record failure\n");
        return 1;
    }
    fprintf(stderr, "PASS bridge I/O: short counts, EINTR, partial EOF/error, no progress and sticky carrier output\n");
    return 0;
}
