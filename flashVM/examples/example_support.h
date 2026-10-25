/* FlashVM tutorial support: report a failed condition and stop the example.
 * Учебные проверки остаются активными при NDEBUG; вызовы API выполняются всегда.
 */

/* Не допускаем повторного определения вспомогательной функции в одном файле. */
#ifndef FLASHVM_EXAMPLE_SUPPORT_H
/* Обозначаем, что объявления этого учебного заголовка уже подключены. */
#define FLASHVM_EXAMPLE_SUPPORT_H /* Include guard for example diagnostics. */
/* Подключаем fprintf и стандартный поток ошибок stderr. */
#include <stdio.h>

/* Подключаем exit и код неуспешного завершения EXIT_FAILURE. */
#include <stdlib.h>


/* Print the failed condition and source location, then exit the tutorial process.
 * condition: проверяемое выражение; file/line: файл и строка диагностики неуспешной проверки.
 * This fail-fast helper does not perform application resource cleanup on failure;
 * successful examples still join workers and release resources explicitly.
 */


/* Объявляем общую функцию диагностики учебного примера. */
static void fvm_example_fail(const char *condition, const char *file, int line)
/* Начинаем тело функции диагностики. */
{ /* Begin scope: Начинаем тело функции диагностики. */
    /* Сообщаем пользователю условие, исходный файл и номер строки ошибки. */
    fprintf(stderr, "FlashVM example failed: %s (%s:%d)\n", condition, file, line);
    /* Завершаем весь учебный процесс с кодом ошибки, в том числе при ошибке callback. */
    exit(EXIT_FAILURE);
    /* Завершаем тело функции; exit выше не возвращается. */
} /* End scope: Начинаем тело функции диагностики. */
/* Завершаем защиту от повторного подключения этого заголовка. */
#endif
