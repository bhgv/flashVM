/* Copyright (c) 2026 bhgv
 * EN: Linux-hosted teaching carrier: two reliable ordered byte endpoints backed
 * only by bounded memory and native synchronization, without OS descriptors.
 * RU: Учебный Linux-носитель: два reliable ordered byte endpoints на ограниченной
 * памяти и native synchronization, без OS-дескрипторов.
 * standalone-reviewed: private example API; no public library ABI is added.
 */
#ifndef EXAMPLE_MEMORY_PHY_H
#define EXAMPLE_MEMORY_PHY_H
#include "phy.h"

#define MEMORY_CAPACITY 16U /* Per-direction bytes; fullness returns AGAIN instead of dropping data. */


/* EN/RU: Caller-owned observations stay alive until both endpoints are destroyed.
 * Наблюдения caller остаются живыми до уничтожения обоих endpoints.
 */
typedef struct MemoryStats
{
    unsigned long reads;        /* Actual driver read callbacks, including AGAIN/EOF. */
    unsigned long writes;       /* Actual driver write callbacks; the wrapper never retries one itself. */
    unsigned long read_bytes;   /* Confirmed delivered prefixes. */
    unsigned long write_bytes;  /* Confirmed accepted prefixes. */
    unsigned long again;        /* Full/empty/contended attempts without progress. */
    unsigned long destroyed;    /* Exactly two after final ownership retirement. */
} MemoryStats;


/* EN: Create two owned endpoints; stats is borrowed through their final destruction
 * and must start zeroed. cut_after=0 disables failure; otherwise endpoint 0 reports
 * ERROR|UNCERTAIN at that accepted-byte boundary. Return 1/0; valid output slots become NULL on construction failure; invalid arguments remain unchanged.
 * Native Linux/pthread/GCC atomics are example dependencies, not wire requirements.
 * RU: Создать два owned endpoints; stats заимствован до final destruction и изначально
 * нулевой. cut_after=0 отключает отказ; иначе endpoint 0 выдаёт ERROR|UNCERTAIN на
 * этой границе принятых байтов. Вернуть 1/0; при construction failure outputs NULL, invalid arguments не изменяются.
 * Linux/pthread/GCC atomics нужны примеру, не wire-протоколу.
 */
int memory_pair(RemotePhy **left, RemotePhy **right, MemoryStats *stats, size_t cut_after);
#endif
