/* Copyright (c) 2026 bhgv
 * EN: POSIX execution and retirement of a fixed, owner-configured command.
 * RU: Запуск и завершение фиксированной команды, заданной владельцем.
 */
#include "launch_process.h"
#include "flashvm/srecord.h"
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ; /* Environment inherited from the owner. */


/* EN/RU: Start only the configured command. After fork the child uses safe POSIX operations.
 * Запустить заданную команду. В дочернем процессе после fork используются безопасные операции POSIX. */
static int start(void *opaque)
{
    StyxLaunchProcess *process = opaque;
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
        execve(process->argv[0], process->argv, environ);
        _exit(127);
    }
    process->child = child;
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
