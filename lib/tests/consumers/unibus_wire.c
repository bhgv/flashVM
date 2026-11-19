/* Copyright (c) 2026 bhgv
 * EN: Installed allocation-free wire link probe using a real checked S-record.
 * RU: Проверка линковки установленного wire codec реальной проверяемой S-записью.
 */
#include <unibus/wire.h>


/* EN: No arguments. Encode one local record; return 0 on OK, 1 on failure.
 * Stack-owned disjoint input/output; no carrier, threads or allocation.
 * RU: Аргументов нет. Кодировать запись; вернуть 0 при OK, 1 при отказе.
 * Раздельные input/output в стеке; без carrier, потоков и allocation.
 */
int main(void)
{
    unsigned char bytes[32]; /* Destination includes checksum, counts, tag and one payload byte. */
    size_t used; /* Encoder publishes the complete output extent only on success. */
    return fvm_srecord_write(1, "x", 1, bytes, sizeof(bytes), &used) != FVM_S_OK; /* Force the installed codec symbol to resolve. */
}
