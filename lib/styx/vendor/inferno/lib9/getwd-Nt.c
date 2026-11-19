/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getwd-Nt support.
 * RU: исходная POSIX-поддержка lib9 getwd-Nt.
 */
#include "lib9.h"
#include <windows.h>

/* [standalone-contract:getwd]
 * EN: getwd: original POSIX lib9 getwd-Nt support.
 * RU: getwd: исходная POSIX-поддержка lib9 getwd-Nt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: char *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *getwd(char *buf, int size)
{
    int n;
    char *p;

    buf[0] = 0;

    n = GetCurrentDirectory(size, buf);
    if (n == 0)
    {
        /*winerror();*/
        return 0;
    }

    for (p = buf; *p; p++)
    {
        if (*p == '\\')
        {
            *p = '/';
        }
    }

    return buf;
}
