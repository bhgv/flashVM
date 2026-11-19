/* Copyright (c) 2026 bhgv
 * EN: Installed SDK link probe: encode-size calculation through the portable 9P API.
 * RU: Проверка линковки SDK: вычислить размер сообщения переносимым 9P API.
 */
#include <styx/codec.h>


/* EN: No arguments. Return 0 if a real codec symbol accepts this Tversion,
 * 1 if it rejects it. Stack-owned input only; no I/O or allocation.
 * RU: Аргументов нет. Вернуть 0, если реальный codec принимает Tversion,
 * 1 при отказе. Вход в стеке; без I/O и allocation.
 */
int main(void)
{
    Fcall message = {0}; /* All unused union/storage fields start cleared. */
    message.type = Tversion; /* Link against the actual version-message encoder size calculation. */
    message.version = (char *)"9P2000"; /* Codec measures this string without modifying it. */
    message.msize = 8192; /* Representative requested message capacity. */
    return sizeS2M(&message) == 0; /* A symbol reference is required; a header-only compile is insufficient. */
}
