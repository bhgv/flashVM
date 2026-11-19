/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 isnan-posix support.
 * RU: исходная POSIX-поддержка lib9 isnan-posix.
 */
#include <lib9.h>

/* [standalone-contract:isNaN]
 * EN: isNaN: original POSIX lib9 isnan-posix support.
 * RU: isNaN: исходная POSIX-поддержка lib9 isnan-posix.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int isNaN(double d)
{
    return isnan(d);
}

/* [standalone-contract:isInf]
 * EN: isInf: original POSIX lib9 isnan-posix support.
 * RU: isInf: исходная POSIX-поддержка lib9 isnan-posix.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * sign (int sign): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int isInf(double d, int sign)
{
    if (sign < 0)
    {
        return isinf(d) < 0;
    }
    return isinf(d);
}
