/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 setbinmode-Nt support.
 * RU: исходная POSIX-поддержка lib9 setbinmode-Nt.
 */
#include "lib9.h"

/* [standalone-contract:setbinmode]
 * EN: setbinmode: original POSIX lib9 setbinmode-Nt support.
 * RU: setbinmode: исходная POSIX-поддержка lib9 setbinmode-Nt.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void setbinmode(void)
{
    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
    _setmode(2, _O_BINARY);
}
