/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 dirwstat support.
 * RU: исходная POSIX-поддержка lib9 dirwstat.
 */
#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>

/* [standalone-contract:dirfwstat]
 * EN: dirfwstat: original POSIX lib9 dirwstat support.
 * RU: dirfwstat: исходная POSIX-поддержка lib9 dirwstat.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dirfwstat(int fd, Dir *d)
{
    return -1;
}

/* [standalone-contract:dirwstat]
 * EN: dirwstat: original POSIX lib9 dirwstat support.
 * RU: dirwstat: исходная POSIX-поддержка lib9 dirwstat.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dirwstat(char *name, Dir *d)
{
    return -1;
}
