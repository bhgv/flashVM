/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getcallerpc-native64 support.
 * RU: исходная POSIX-поддержка lib9 getcallerpc-native64.
 */
#include "lib9.h"

/* Obtain a diagnostic return PC through the compiler's target ABI, including AArch64 PAC. */


/* [standalone-contract:getcallerpc]
 * EN: getcallerpc: original POSIX lib9 getcallerpc-native64 support.
 * RU: getcallerpc: исходная POSIX-поддержка lib9 getcallerpc-native64.
 * unused (void *unused): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: uintptr; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uintptr; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uintptr getcallerpc(void *unused)
{
    (void)unused;
    return (uintptr)__builtin_extract_return_addr(__builtin_return_address(0));
}
