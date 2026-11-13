/* Copyright (c) 2026 bhgv
 * [GC:] EN: Private negative control restores raw builtin scalar result stores.
 * RU: Приватный negative control возвращает raw builtin scalar result stores.
 */
#include <stddef.h>
#include <string.h>


/* [GC:] EN: Deliberately omit cell guards for borrowed private snapshot/mapped
 * destination. Only TSan's actual conflicting accesses count as detection.
 * RU: Намеренно пропустить cell guards для borrowed private snapshot/mapped
 * destination. Лишь настоящие conflicting accesses TSan засчитываются. */
void __wrap_dvm_sys_scalar_result(const void *snapshot, void *destination, size_t size)
{
    memmove(destination, snapshot, size);
}
