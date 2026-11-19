/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 nulldir support.
 * RU: исходная POSIX-поддержка lib9 nulldir.
 */
#include "lib9.h"

/* [standalone-contract:nulldir]
 * EN: nulldir: original POSIX lib9 nulldir support.
 * RU: nulldir: исходная POSIX-поддержка lib9 nulldir.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void nulldir(Dir *d)
{
    memset(d, ~0, sizeof(Dir));
    d->name = d->uid = d->gid = d->muid = "";
}
