/* Copyright (c) 2026 bhgv
 * EN: Private arena owner and stable native allocation identities; include lib9.h/pool.h first.
 * RU: Частный владелец арены и стабильные native-идентификаторы; сначала lib9.h/pool.h.
 */
#ifndef DVM_HEAP_POOL_H
#define DVM_HEAP_POOL_H
#include "channel_sync.h"
#include "disvm/heap_settings.h"


/* [GC:] EN: Private native owner published only after complete initialization.
 * sync protects cursor/headers; raw GC walks require independent quiescence.
 * RU: Частный native-владелец публикуется после полной инициализации. sync
 * защищает курсор/заголовки; сырой GC-обход требует отдельного покоя. */
struct Pool
{
    Bhdr *arena;        /* Owned arena/header, stable until quiescent destruction. */
    ulong bytes;        /* Native checked extent, not a serialized VM field. */
    Bhdr *cursor;       /* Next-fit search boundary, under sync; block addresses never move. */
    FVM_PortSync *sync; /* Protects allocation headers, not managed payload. */
};


/* [GC:] EN: Initialize zeroed private pool with bounded/aligned bytes before
 * users start. Return zero unchanged on refusal, one transferring arena/mutex.
 * RU: Создать нулевой pool размера bytes до запуска потоков. Размер ограничен/
 * выровнен. Ноль — отказ без изменений, один — передача арены/мьютекса. */
int dvm_heap_pool_init(Pool *pool, ulong bytes);


/* [GC:] EN: Destroy only independently quiescent pool with no live blocks;
 * return zero unchanged for live/corrupt/uninitialized state. Caller joins all
 * users/raw walkers first; mutex cannot protect its own lifetime.
 * RU: Удалить отдельно успокоенный pool без живых блоков; вернуть ноль без
 * изменений при живом/повреждённом/неинициализированном состоянии. Сначала
 * присоединить пользователей/обходы; мьютекс не защищает собственную жизнь. */
int dvm_heap_pool_destroy(Pool *pool);


/* [GC:] EN: Try size payload bytes from live pool; return native storage or NULL.
 * No VM callback runs; native block headers/cursor use the pool mutex.
 * RU: Пробовать size байт в живом pool; вернуть native-память либо NULL. Callback
 * VM не вызывается; заголовки/курсор защищены мьютексом. */
void *dvm_heap_pool_try_alloc(Pool *pool, ulong size);


/* [GC:] EN: Query generation for independently owned value in pool under mutex.
 * Return zero if invalid. Caller pins storage throughout; this is no retain.
 * RU: Прочитать поколение собственного value в pool под мьютексом. Ноль —
 * неверное выделение; вызывающий удерживает память, retain здесь нет. */
DVM_HeapGeneration dvm_heap_pool_generation(Pool *pool, void *value);


/* [GC:] EN: Capture metadata identity of independently owned value in pool.
 * Return zero key if invalid; no ownership or stale-key dereference is granted.
 * RU: Получить метаданные идентичности собственного value в pool. Нулевой ключ —
 * неверный ввод; владение и доступ по устаревшему ключу не даются. */
DVM_HeapKey dvm_heap_pool_key(Pool *pool, void *value);


/* [GC:] EN: Compare key to independently live value in pool; return boolean
 * without retaining. Caller keeps allocation alive through the generation query.
 * RU: Сравнить key с отдельно живым value в pool; вернуть bool без retain.
 * Вызывающий хранит выделение до конца чтения поколения. */
int dvm_heap_pool_key_matches(Pool *pool, void *value, DVM_HeapKey key);
#endif
