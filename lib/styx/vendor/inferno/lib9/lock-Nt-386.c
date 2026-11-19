/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 lock-Nt-386 support.
 * RU: исходная POSIX-поддержка lib9 lock-Nt-386.
 */
#include "lib9.h"

/* [standalone-contract:_tas]
 * EN: _tas: original POSIX lib9 lock-Nt-386 support.
 * RU: _tas: исходная POSIX-поддержка lib9 lock-Nt-386.
 * la (int *la): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _tas(int *la)
{
    int v;

    _asm {
		mov eax, la
		mov ebx, 1
		xchg	ebx, [eax]
		mov	v, ebx
    }
    return v;
}
