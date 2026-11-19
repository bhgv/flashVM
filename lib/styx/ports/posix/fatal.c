/* Copyright (c) 2026 bhgv
 * EN: Original library invariant failure endpoint, independent of any VM.
 * RU: Library invariant failure endpoint, независимо от VM.
 */
#include <stdio.h>
#include <stdlib.h>

/* EN/RU: Report an upstream invariant and terminate; no allocation or formatter recursion.
 * Сообщить upstream invariant и завершить; без allocation/formatter recursion. */


/* [standalone-contract:_assert]
 * EN: _assert: posix/fatal implementation and native state.
 * RU: _assert: реализация posix/fatal и нативное состояние.
 * message (char *message): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _assert(char *message)
{
    fprintf(stderr, "Inferno library invariant: %s\n", message);
    abort();
}
