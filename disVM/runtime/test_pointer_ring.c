/* Copyright (c) 2026 bhgv
 * EN: Observe a real committed pointer-ring consume without changing VM behavior.
 * RU: Наблюдать настоящую транзакцию consume pointer-кольца без изменения VM.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "shared_mapped.h"
#include <stdio.h>

static int observed; /* Atomic one-time evidence after an actual guarded consume. */


/* EN: Linker target with the production ABI; all arguments remain borrowed.
 * RU: Цель линкера с production ABI; все аргументы остаются заимствованными. */
int __real_dvm_runtime_mapped_buffer_transfer(const void *source,
                                              void *destination,
                                              Type *type,
                                              int consume,
                                              int *committed,
                                              int replay_opcode,
                                              const Prog *sender);


/* [GC:] EN: Forward the real transfer before observing a committed Tptr consume.
 * source/destination/type/sender retain their production ownership; committed is
 * caller-owned output, consume/replay_opcode retain their meanings. Return the
 * exact result; exceptions propagate. No added VM locks, pauses or owner changes.
 * Atomic evidence is safe for concurrent resident calls; report only once.
 * RU: Сначала выполнить настоящую передачу, затем отметить committed consume Tptr.
 * Владение source/destination/type/sender неизменно; committed — выход caller,
 * consume/replay_opcode имеют исходный смысл. Вернуть исходный результат; ошибки
 * передаются дальше. Без новых VM lock, пауз и смены владельцев. Атомарная отметка
 * безопасна при одновременных вызовах; сообщение выводится однократно. */
int __wrap_dvm_runtime_mapped_buffer_transfer(const void *source,
                                              void *destination,
                                              Type *type,
                                              int consume,
                                              int *committed,
                                              int replay_opcode,
                                              const Prog *sender)
{
    int result =
        __real_dvm_runtime_mapped_buffer_transfer(source, destination, type, consume, committed, replay_opcode, sender);
    if (type == &Tptr && consume && *committed && !__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "PASS pointer ring guarded consume\n");
    }
    return result;
}
