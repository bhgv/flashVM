/* Copyright (c) 2026 bhgv
 * EN: POSIX execution and retirement of a fixed, owner-configured command.
 * RU: Запуск и завершение фиксированной команды, заданной владельцем.
 */
#define _GNU_SOURCE
#include "launch_process.h"
#include "flashvm/srecord.h"
#include "launch/ticket.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ; /* Environment inherited from the owner. */


/* EN/RU: Start only the configured command. After fork the child uses safe POSIX operations.
 * Запустить заданную команду. В дочернем процессе после fork используются безопасные операции POSIX. */
static int spawn_process(StyxLaunchProcess *process, char *const *argv, int startup_fd)
{
    pid_t child = fork();
    if (child < 0)
    {
        return 0;
    }
    if (!child)
    {
        if ((startup_fd >= 0 && fcntl(startup_fd, F_SETFD, 0) < 0) || chdir(process->directory))
        {
            _exit(126);
        }
        execve(argv[0], argv, environ);
        _exit(127);
    }
    process->child = child;
    return 1;
}


/* EN/RU: Start a fixed command with its original owner-provided argv.
 * Запустить фиксированную команду с исходными аргументами владельца. */
static int start(void *opaque)
{
    StyxLaunchProcess *process = opaque;
    return spawn_process(process, process->argv, -1);
}


/* EN/RU: Stage an immutable startup payload in a close-on-exec Linux memfd.
 * Подготовить неизменяемые стартовые данные в Linux memfd с close-on-exec. */
static int startup_file(const void *data, size_t size)
{
    int fd = memfd_create("disvm-startup", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0)
    {
        return -1;
    }
    if (fd < 3)
    {
        int moved = fcntl(fd, F_DUPFD_CLOEXEC, 3);
        close(fd);
        if (moved < 0)
        {
            return -1;
        }
        fd = moved;
    }
    const unsigned char *bytes = data;
    size_t written = 0;
    while (written < size)
    {
        ssize_t n = write(fd, bytes + written, size - written);
        if (n > 0)
        {
            written += (size_t)n;
        }
        else if (n == 0 || errno != EINTR)
        {
            close(fd);
            return -1;
        }
    }
    if (fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}


/* EN/RU: Put only the private descriptor number in the DisVM argument vector.
 * Передать в командной строке DisVM только номер частного дескриптора. */
static int start_payload(void *opaque, const void *data, size_t size)
{
    StyxLaunchProcess *process = opaque;
    size_t count = 0;
    while (process->argv[count])
    {
        count++;
    }
    char **argv = calloc(count + 2, sizeof(*argv));
    if (!argv)
    {
        return 0;
    }
    int fd = startup_file(data, size);
    if (fd < 0)
    {
        free(argv);
        return 0;
    }
    char option[40];
    snprintf(option, sizeof(option), "--startup-fd=%d", fd);
    argv[0] = process->argv[0];
    argv[1] = option;
    memcpy(argv + 2, process->argv + 1, count * sizeof(*argv));
    int started = spawn_process(process, argv, fd);
    close(fd);
    free(argv);
    return started;
}


/* EN/RU: Enable bounded binary startup arguments for the configured DisVM executable.
 * Разрешить ограниченные бинарные аргументы для заданного исполняемого DisVM. */
int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit)
{
    if (!process || process->receiver.start != start || process->receiver.used || limit > DVM_LAUNCH_ARGUMENT_LIMIT)
    {
        return 0;
    }
    process->receiver.start_payload = start_payload;
    process->receiver.payload_limit = limit;
    return 1;
}


/* EN/RU: See launch_process.h; configuration supplies all execution authority.
 * См. launch_process.h; все параметры исполнения задаёт владелец. */
int styxlaunchprocessinit(StyxLaunchProcess *process, unsigned long tag,
    const char *directory, char *const *argv)
{
    if (!process || !tag || tag > FVM_SRECORD_U32 || !directory || !*directory ||
        !argv || !argv[0] || argv[0][0] != '/')
    {
        return 0;
    }
    memset(process, 0, sizeof(*process));
    process->directory = directory;
    process->argv = argv;
    process->receiver = (StyxLaunchReceiver){.tag = tag, .start = start, .state = process};
    return 1;
}


/* EN/RU: See launch_process.h; consume the kernel status once and cache it.
 * См. launch_process.h; однократно получить статус ОС и сохранить его. */
int styxlaunchprocessstatus(StyxLaunchProcess *process, int wait, int *status)
{
    if (!process || !status)
    {
        return -1;
    }
    if (!process->completed && process->child > 0)
    {
        int raw;
        pid_t result;
        do
        {
            result = waitpid(process->child, &raw, wait ? 0 : WNOHANG);
        }
        while (result < 0 && errno == EINTR);
        if (!result)
        {
            return 0;
        }
        process->child = 0;
        process->completed = 1;
        process->status = result < 0 ? -1 : WIFEXITED(raw) ? WEXITSTATUS(raw) :
            WIFSIGNALED(raw) ? 128 + WTERMSIG(raw) : -1;
    }
    if (!process->completed)
    {
        return 0;
    }
    if (process->status < 0)
    {
        return -1;
    }
    *status = process->status;
    return 1;
}
