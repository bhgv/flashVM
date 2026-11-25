/* Copyright (c) 2026 bhgv
 * EN: Delegate a private connected stream to a companion process and retire it.
 * RU: Выдать встречному процессу закрытый поток и дождаться его завершения.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LOCAL_PAIR_RETIRE_POLLS 200 /* Give the companion two seconds to retire after EOF. */
#define LOCAL_PAIR_POLL_NS 10000000L /* Poll waitpid without blocking the timeout. */
#define LOCAL_PAIR_FD_MIN 3         /* Preserve the standard streams in both processes. */

extern int dvm_styx_mount_cli(int, char **);


/* EN: Reap the exact companion, allowing a bounded normal exit after EOF.
 * RU: Дождаться именно встречного процесса, ограничив время завершения после EOF. */
static int retire_peer(pid_t child)
{
    const struct timespec pause = {0, LOCAL_PAIR_POLL_NS};
    int status;
    for (unsigned attempt = 0; attempt < LOCAL_PAIR_RETIRE_POLLS; ++attempt)
    {
        pid_t finished = waitpid(child, &status, WNOHANG);
        if (finished == child)
        {
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
        if (finished < 0 && errno != EINTR)
        {
            return 1;
        }
        nanosleep(&pause, NULL);
    }
    kill(child, SIGKILL);
    while (waitpid(child, &status, 0) < 0 && errno == EINTR)
    {
        /* EN: Only our child owns this PID until it is reaped. */
    }
    fputs("disvm: Styx companion failed to retire\n", stderr);
    return 1;
}


/* EN: Run the same Dis mount handler on one socketpair end. The executable
 * named by argv receives --fd and its own copied options on the other end.
 * RU: Запустить обычный монтирующий обработчик Dis на одном конце socketpair.
 * Программа из argv получает --fd и свои аргументы на другом конце. */
int dvm_styx_local_pair_cli(int argc, char **argv)
{
    int split = 3;
    while (split < argc && strcmp(argv[split], "--"))
    {
        ++split;
    }
    if (split + 2 >= argc || !argv[2][0])
    {
        fputs("usage: disvm --styx-local-pair PROGRAM [PEER OPTIONS] -- [MOUNT OPTIONS] -- [VM OPTIONS] BOOTSTRAP.dis\n", stderr);
        return 2;
    }
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pair))
    {
        perror("disvm: local Styx socketpair");
        return 1;
    }
    int client = fcntl(pair[0], F_DUPFD_CLOEXEC, LOCAL_PAIR_FD_MIN);
    int provider = fcntl(pair[1], F_DUPFD_CLOEXEC, LOCAL_PAIR_FD_MIN);
    close(pair[0]);
    close(pair[1]);
    if (client < 0 || provider < 0)
    {
        if (client >= 0)
        {
            close(client);
        }
        if (provider >= 0)
        {
            close(provider);
        }
        fputs("disvm: cannot allocate companion descriptors\n", stderr);
        return 1;
    }
    char client_text[32], provider_text[32];
    snprintf(client_text, sizeof(client_text), "%d", client);
    snprintf(provider_text, sizeof(provider_text), "%d", provider);
    char *peer_args[split + 1];
    peer_args[0] = argv[2];
    peer_args[1] = "--fd";
    peer_args[2] = provider_text;
    memcpy(peer_args + 3, argv + 3, (size_t)(split - 3) * sizeof(*argv));
    peer_args[split] = NULL;
    pid_t parent = getpid();
    pid_t child = fork();
    if (!child)
    {
        close(client);
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) || getppid() != parent || fcntl(provider, F_SETFD, 0) < 0)
        {
            _exit(126);
        }
        execv(peer_args[0], peer_args);
        perror("disvm: exec Styx companion");
        _exit(127);
    }
    close(provider);
    if (child < 0)
    {
        perror("disvm: fork Styx companion");
        close(client);
        return 1;
    }
    int mount_count = argc - split + 2;
    char *mount_args[mount_count + 1];
    mount_args[0] = argv[0];
    mount_args[1] = "--styx-mount-fd";
    mount_args[2] = client_text;
    memcpy(mount_args + 3, argv + split + 1, (size_t)(argc - split) * sizeof(*argv));
    int result = dvm_styx_mount_cli(mount_count, mount_args);
    close(client);
    int peer_result = retire_peer(child);
    return result ? result : peer_result;
}
