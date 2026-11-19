/* Copyright (c) 2026 bhgv
 * EN: Strict CLI for the wire-only gateway; preserve the one-port form and allow
 * separate client ports. stdout belongs exclusively to binary S-record traffic.
 * RU: Строгий CLI wire-only шлюза; один общий либо два отдельных порта клиентов.
 * stdout принадлежит только двоичному потоку S-записей.
 * standalone-reviewed: authored command-line and ownership contracts.
 */
#include "bridge.h" /* Private example coordinator; no VM or libstyx API. */
#include <signal.h> /* Broken output must return EPIPE instead of terminating a worker silently. */
#include <stdio.h>  /* Usage/errors go to stderr, never into the carrier. */


/* EN: Parse borrowed text as digits only, 1..65535; publish port only on success.
 * Return 1/0; no allocation, locale dependence, signs or whitespace accepted.
 * RU: Разобрать text как цифры 1..65535; опубликовать port только при успехе.
 * Вернуть 1/0; без allocation, locale, знаков и пробелов.
 */
static int parse_port(const char *text, unsigned *port)
{
    unsigned value = 0; /* Bound before each multiplication, so long input cannot wrap. */
    if (!text || !*text)
    {
        return 0; /* An empty argument must not silently select an endpoint. */
    }
    for (; *text; text++)
    {
        if (*text < '0' || *text > '9' || value > (65535U - (unsigned)(*text - '0')) / 10U)
        {
            return 0; /* Reject signs, trailing junk, whitespace and overflow uniformly. */
        }
        value = value * 10U + (unsigned)(*text - '0'); /* Consume only a validated decimal digit. */
    }
    if (!value)
    {
        return 0; /* Port zero is not a connectable backend endpoint. */
    }
    *port = value; /* Caller output remains unchanged for every rejected argument. */
    return 1;
}


/* EN: argc/argv are borrowed process arguments: PORT7 [PORT8]. One port connects
 * both independent sessions to the same listener. Return 1 for usage/setup of
 * signal handling; bridge_run returns 0/2 for normal/failing session completion.
 * RU: argc/argv — аргументы процесса: PORT7 [PORT8]. Один порт подключает обе
 * независимые сессии к одному listener. Вернуть 1 при usage/signal error;
 * bridge_run возвращает 0/2 при успешном/ошибочном завершении сессии.
 */
int main(int argc, char **argv)
{
    unsigned ports[BRIDGE_CLIENTS]; /* Endpoint selection is separate from the two clients' fid/tag spaces. */
    if ((argc != 2 && argc != 3) || !parse_port(argv[1], &ports[0]) ||
        (argc == 3 && !parse_port(argv[2], &ports[1])))
    {
        fprintf(stderr, "usage: %s PORT7 [PORT8] (decimal 1..65535)\n", argv[0]); /* This must never appear in stdout. */
        return 1;
    }
    if (argc == 2)
    {
        ports[1] = ports[0]; /* Preserve the original one-listener invocation used by 9P integration tests. */
    }
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("ignore SIGPIPE"); /* A worker must observe failed writes to report transport failure. */
        return 1;
    }
    return bridge_run(ports); /* Session coordinator owns all resources it creates. */
}
