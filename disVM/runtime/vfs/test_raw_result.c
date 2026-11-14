/* Copyright (c) 2026 bhgv
 * EN: Test-only omission of scalar publication guards; never linked in runtime.
 * RU: Только тестовый пропуск guards scalar publication; отсутствует в runtime.
 */
#include <stddef.h>
#include <string.h>


/* EN: Copy borrowed scalar bytes into the shared destination without guards.
 * This intentional race must be detected by TSan, not accepted as normal exit.
 * RU: Скопировать borrowed scalar bytes в shared destination без guards.
 * Намеренная гонка обязана обнаруживаться TSan, штатный выход не принимается. */
void __wrap_dvm_sys_scalar_result(const void *snapshot, void *destination, size_t size)
{
    memmove(destination, snapshot, size);
}
