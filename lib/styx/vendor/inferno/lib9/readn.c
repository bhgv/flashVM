/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 readn support.
 * RU: исходная POSIX-поддержка lib9 readn.
 */
#include "lib9.h"

/* [standalone-contract:readn]
 * EN: readn: original POSIX lib9 readn support.
 * RU: readn: исходная POSIX-поддержка lib9 readn.
 * f (int f): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * av (void *av): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
long readn(int f, void *av, long n)
{
    char *a;
    long m, t;

    a = av;
    t = 0;
    while (t < n)
    {
        m = read(f, a + t, n - t);
        if (m <= 0)
        {
            if (t == 0)
            {
                return m;
            }
            break;
        }
        t += m;
    }
    return t;
}
