/* Copyright (c) 2026 bhgv
 * EN: Native formatter registry lock; callbacks execute after releasing it.
 * RU: Native formatter registry lock; callbacks выполняются после unlock.
 */
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t fmtmutex = PTHREAD_MUTEX_INITIALIZER; /* Short native registry transactions. */


/* EN/RU: Lock formatter metadata; no VM execution or callbacks.
 * Захватить formatter metadata; без VM execution/callbacks. */
void
_fmtlock(void)
{
	if(pthread_mutex_lock(&fmtmutex) != 0)
		abort();
}


/* EN/RU: Release metadata before invoking the selected formatter.
 * Освободить metadata до вызова formatter. */
void
_fmtunlock(void)
{
	if(pthread_mutex_unlock(&fmtmutex) != 0)
		abort();
}
