/* Copyright (c) 2026 bhgv
 * EN: C90 streaming endpoint for independent named-service byte vectors, no VM or transport.
 * RU: C90-интерфейс независимых байтовых векторов службы, без VM и транспорта.
 */
#include "flashvm/sordctl.h"
#include <stdio.h>


/* EN/RU: Read ten scalar parts from stdin and emit exact named-body hex; stop on any codec/I/O failure.
 * Читать десять скалярных частей и выводить точный hex тела; завершиться при отказе кодека/I/O.
 */
int main(void)
{
    FVM_SOrderControl message; /* Input portable fields. */
    unsigned char bytes[FVM_SORDER_CTL_MAX]; /* One complete named-service body. */
    size_t size; /* Exact encoded extent. */
    size_t at; /* Output byte cursor. */
    while (scanf("%lu %lx %lx %lx %lu %lx %lx %lx %lx %lx", &message.command,
        &message.operation.high, &message.operation.low, &message.lane, &message.limit,
        &message.window.high, &message.window.low, &message.tag, &message.boundary.high, &message.boundary.low) == 10)
    {
        if (fvm_sorder_control_write(&message, bytes, sizeof(bytes), &size) != FVM_S_OK)
        {
            return 1;
        }
        for (at = 0; at < size; at++)
        {
            printf("%02x", bytes[at]);
        }
        putchar('\n');
    }
    return ferror(stdin) || ferror(stdout);
}
