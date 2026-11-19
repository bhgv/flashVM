/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getfields support.
 * RU: исходная POSIX-поддержка lib9 getfields.
 */
#include "lib9.h"

/* [standalone-contract:getfields]
 * EN: getfields: original POSIX lib9 getfields support.
 * RU: getfields: исходная POSIX-поддержка lib9 getfields.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (char **args): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * max (int max): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mflag (int mflag): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * set (char *set): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int getfields(char *str, char **args, int max, int mflag, char *set)
{
    Rune r;
    int nr, intok, narg;

    if (max <= 0)
    {
        return 0;
    }

    narg = 0;
    args[narg] = str;
    if (!mflag)
    {
        narg++;
    }
    intok = 0;
    for (;; str += nr)
    {
        nr = chartorune(&r, str);
        if (r == 0)
        {
            break;
        }
        if (utfrune(set, r))
        {
            if (narg >= max)
            {
                break;
            }
            *str = 0;
            intok = 0;
            args[narg] = str + nr;
            if (!mflag)
            {
                narg++;
            }
        }
        else
        {
            if (!intok && mflag)
            {
                narg++;
            }
            intok = 1;
        }
    }
    return narg;
}
