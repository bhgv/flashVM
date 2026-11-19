/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 cistrncmp support.
 * RU: исходная POSIX-поддержка lib9 cistrncmp.
 */
#include "lib9.h"

/* Compare unsigned bytes with ASCII case folding, without reading past a nonpositive bound. */


/* [standalone-contract:cistrncmp]
 * EN: cistrncmp: original POSIX lib9 cistrncmp support.
 * RU: cistrncmp: исходная POSIX-поддержка lib9 cistrncmp.
 * s1 (char *s1): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * s2 (char *s2): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int cistrncmp(char *s1, char *s2, int n)
{
    int c1, c2;

    while (n > 0)
    {
        c1 = *(uchar *)s1++;
        c2 = *(uchar *)s2++;
        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 += 'a' - 'A';
        }
        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 += 'a' - 'A';
        }
        if (c1 != c2 || c1 == 0)
        {
            return c1 - c2;
        }
        n--;
    }
    return 0;
}
