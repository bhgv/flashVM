/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 getuser-Nt support.
 * RU: исходная POSIX-поддержка lib9 getuser-Nt.
 */
#include "lib9.h"

/* [standalone-contract:getuser]
 * EN: getuser: original POSIX lib9 getuser-Nt support.
 * RU: getuser: исходная POSIX-поддержка lib9 getuser-Nt.
 * Parameters: none. Параметры: нет.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *getuser(void)
{
    /* could do better, but result isn't really used */
    return "unknown";
}
