/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getwd-posix support.
 * RU: исходная POSIX-поддержка lib9 getwd-posix.
 */
#include "lib9.h"
#undef getwd
#include <unistd.h>

/* [standalone-contract:infgetwd]
 * EN: infgetwd: original POSIX lib9 getwd-posix support.
 * RU: infgetwd: исходная POSIX-поддержка lib9 getwd-posix.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *infgetwd(char *buf, int size)
{
    return getcwd(buf, size);
}
