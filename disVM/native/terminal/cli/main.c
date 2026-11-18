/* Copyright (c) 2026 bhgv
 * EN: Explicit native Unibus relay CLI, binary application stdout and diagnostics on stderr only.
 * RU: Явный native CLI-relay Unibus, двоичный клиентский stdout и диагностика лишь в stderr.
 */
#include "session.h"
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


/* EN/RU: Print help before any carrier acquisition or binary output; no implicit remote connection.
 * Вывести help до получения carrier и двоичного output; не создавать неявных удалённых соединений.
 */
void unibuscli_help(void)
{
    puts("Usage: unibus-bidir --profile ordered --phy-fd FD --lane ID --tag ID [--tag ID ...]");
    puts("       unibus-bidir [--profile none] | --help");
    puts("Options: --channel 0|1 --root PATH --rx PATH --tx PATH --limit BYTES");
    puts("         --window NUMBER --timeout-ms MS --receive-records COUNT");
    puts("FD>=3 delegates an already connected reliable ordered stream; both peers select P9MX v1.");
    puts("stdin/stdout carry ordinary application S-records, never outer transport root0.");
    puts("Inner tag0 is application-local when explicitly granted. stderr carries diagnostics.");
    puts("Clean stdin EOF ends only TX input. COUNT=0 receives until transport retirement.");
    puts("The unused P9MX session is closed; this tool exports no reverse filesystem and launches no guest VM.");
}


/* EN/RU: Validate complete CLI before touching descriptors; none profile leaves stdio untouched, ordered owns native workers.
 * Проверить весь CLI до descriptors; профиль none не трогает stdio, ordered создаёт native-workers.
 */
int main(int argc, char **argv)
{
    UnibusCliConfig config; /* Owned parsed native settings. */
    const char *error = "invalid command"; /* Parameter-only diagnostic, never private wire content. */
    if (!unibuscli_parse(argc, argv, &config, &error))
    {
        fprintf(stderr, "unibus-bidir: invalid option/configuration: %s\n", error);
        unibuscli_clear(&config);
        return 2;
    }
    if (config.help || config.profile == UNIBUS_CLI_NONE)
    {
        if (config.help)
        {
            unibuscli_help();
        }
        unibuscli_clear(&config);
        return 0;
    }
    int input_flags = fcntl(STDIN_FILENO, F_GETFL); /* Standard descriptors must exist before any private native pipe is allocated. */
    int output_flags = fcntl(STDOUT_FILENO, F_GETFL); /* Shared stdio retains its descriptor numbers. */
    if (input_flags < 0 || output_flags < 0 || fcntl(STDERR_FILENO, F_GETFL) < 0 ||
        (input_flags & O_ACCMODE) == O_WRONLY || (output_flags & O_ACCMODE) == O_RDONLY)
    {
        fputs("unibus-bidir: stdin/stdout/stderr must be open with appropriate access\n", stderr);
        unibuscli_clear(&config);
        return 2;
    }
    struct sigaction action = {0}; /* Broken application pipe is a native error, not process-wide SIGPIPE termination. */
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL))
    {
        unibuscli_clear(&config);
        return 2;
    }
    sigset_t controls; /* All subsequently created workers delegate process-control signals to one native sigwait owner. */
    sigset_t previous; /* Caller mask restored after native owners and stdio mode retire. */
    sigemptyset(&controls);
    sigaddset(&controls, SIGINT);
    sigaddset(&controls, SIGTERM);
    sigaddset(&controls, SIGUSR1);
    if (pthread_sigmask(SIG_BLOCK, &controls, &previous))
    {
        unibuscli_clear(&config);
        return 2;
    }
    UnibusCliLink link; /* Own connection references only after explicit delegated descriptor adoption. */
    if (!unibuscli_connect(&config, &link))
    {
        fputs("unibus-bidir: cannot establish selected terminal profile\n", stderr);
        unibuscli_clear(&config);
        pthread_sigmask(SIG_SETMASK, &previous, NULL);
        return 2;
    }
    int result = unibuscli_run(link.peer, &config); /* Native relay, application bytes remain opaque. */
    unibuscli_disconnect(&link);
    unibuscli_clear(&config);
    pthread_sigmask(SIG_SETMASK, &previous, NULL);
    if (result && result < 128)
    {
        fputs("unibus-bidir: application stream or terminal connection failed\n", stderr);
    }
    return result;
}
