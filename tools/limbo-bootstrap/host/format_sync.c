/* Copyright (c) 2026 bhgv
 * EN: Protect native formatter registry metadata without locking callback execution.
 * RU: Защитить native-реестр форматтеров, не удерживая mutex при выполнении callback.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t format_mutex = PTHREAD_MUTEX_INITIALIZER; /* Registry entries/count only. */


/* EN: Reject impossible mutex failures using copied status, without lib9 formatting.
 * RU: Завершить при невозможной ошибке mutex по status, не вызывая lib9 formatter. */
static void checked(int status)
{
    if (status)
    {
        fprintf(stderr, "native formatter mutex failure: %d\n", status);
        abort();
    }
}


/* EN: Enter the short native registry transaction; no parameters or callbacks.
 * RU: Войти в короткую операцию реестра; без параметров или callback. */
void _fmtlock(void)
{
    checked(pthread_mutex_lock(&format_mutex));
}


/* EN: Leave registry metadata before invoking any borrowed format callback.
 * RU: Отпустить метаданные реестра до вызова заимствованного format callback. */
void _fmtunlock(void)
{
    checked(pthread_mutex_unlock(&format_mutex));
}
