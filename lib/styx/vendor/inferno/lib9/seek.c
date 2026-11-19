/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 seek support.
 * RU: исходная POSIX-поддержка lib9 seek.
 */
#include "lib9.h"
#include <sys/types.h>
#include <fcntl.h>

/* Keep the full signed offset when calling the Windows CRT seek entry point. */


/* [standalone-contract:seek]
 * EN: seek: original POSIX lib9 seek support.
 * RU: seek: исходная POSIX-поддержка lib9 seek.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * where (vlong where): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * from (int from): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: vlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: vlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
vlong seek(int fd, vlong where, int from)
{
#ifdef _WIN32
    return _lseeki64(fd, where, from);
#else
    return lseek(fd, where, from);
#endif
}
