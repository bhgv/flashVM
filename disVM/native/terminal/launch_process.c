/* Copyright (c) 2026 bhgv
 * EN: POSIX execution and retirement of a fixed, owner-configured command.
 * RU: Запуск и завершение фиксированной команды, заданной владельцем.
 */
#include "launch_process.h"
#include "flashvm/srecord.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ; /* Environment inherited from the owner. */


/* EN/RU: Start only the configured command. After fork the child uses safe POSIX operations.
 * Запустить заданную команду. В дочернем процессе после fork используются безопасные операции POSIX. */
static int spawn_process(StyxLaunchProcess *process, char *const *argv)
{
    pid_t child = fork();
    if (child < 0)
    {
        return 0;
    }
    if (!child)
    {
        if (chdir(process->directory))
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
    return spawn_process(process, process->argv);
}


/* EN/RU: Encode startup bytes as a single bounded DisVM option.
 * Передать стартовые байты одним ограниченным параметром DisVM. */
static int start_payload(void *opaque, const void *data, size_t size)
{
    StyxLaunchProcess *process = opaque;
    const unsigned char *bytes = data;
    static const char digits[] = "0123456789abcdef"; /* Lowercase DisVM hex alphabet. */
    size_t count = 0;
    while (process->argv[count])
    {
        count++;
    }
    char **argv = calloc(count + 2, sizeof(*argv));
    char *encoded = malloc(15 + 2 * size);
    if (!argv || !encoded)
    {
        free(argv);
        free(encoded);
        return 0;
    }
    memcpy(encoded, "--startup-hex=", 14);
    for (size_t i = 0; i < size; i++)
    {
        encoded[14 + 2 * i] = digits[bytes[i] >> 4];
        encoded[15 + 2 * i] = digits[bytes[i] & 15];
    }
    encoded[14 + 2 * size] = 0;
    argv[0] = process->argv[0];
    argv[1] = encoded;
    memcpy(argv + 2, process->argv + 1, count * sizeof(*argv));
    int started = spawn_process(process, argv);
    free(encoded);
    free(argv);
    return started;
}


/* EN/RU: Enable bounded binary startup arguments for the configured DisVM executable.
 * Разрешить ограниченные бинарные аргументы для заданного исполняемого DisVM. */
int styxlaunchprocesspayload(StyxLaunchProcess *process, size_t limit)
{
    if (!process || process->receiver.start != start || process->receiver.used || limit > 32768)
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
