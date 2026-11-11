/* Copyright (c) 2026 bhgv
 * EN: Serialize UTF-8 cache metadata; published unchanged views remain read-only.
 * RU: Сериализовать метаданные UTF-8; опубликованные неизменные views только читаются.
 */
#include "string_view_sync.h"
#include <pthread.h>

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER; /* No VM work, GC waits or callbacks under this mutex. */


/* [GC:] EN: Acquire native cache protection for the caller's rooted string.
 * RU: Захватить нативную защиту кэша укоренённой строки вызывающего. */
void dvm_string_view_lock(void)
{
    pthread_mutex_lock(&cache_mutex);
}


/* [GC:] EN: Publish complete cache bytes and release ordinary metadata exclusion.
 * RU: Опубликовать готовые байты кэша и снять обычную защиту метаданных. */
void dvm_string_view_unlock(void)
{
    pthread_mutex_unlock(&cache_mutex);
}
