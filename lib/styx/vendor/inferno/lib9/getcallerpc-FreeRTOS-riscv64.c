/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getcallerpc-FreeRTOS-riscv64 support.
 * RU: исходная POSIX-поддержка lib9 getcallerpc-FreeRTOS-riscv64.
 */
#include <lib9.h>

#if 0 //{}






/* [standalone-contract:getcallerpc]
 * EN: getcallerpc: original POSIX lib9 getcallerpc-FreeRTOS-riscv64 support.
 * RU: getcallerpc: исходная POSIX-поддержка lib9 getcallerpc-FreeRTOS-riscv64.
 * x (void *x): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: ulong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: ulong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
ulong
getcallerpc(void *x)
{
ulong *lp;

	lp = x;

	return lp[-1];
}
#endif
