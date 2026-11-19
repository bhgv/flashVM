/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 create support.
 * RU: исходная POSIX-поддержка lib9 create.
 */
#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Preserve exclusive creation and descriptor inheritance flags in native opens. */


/* [standalone-contract:create]
 * EN: create: original POSIX lib9 create support.
 * RU: create: исходная POSIX-поддержка lib9 create.
 * f (char *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * perm (int perm): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int create(char *f, int mode, int perm)
{
    int m, fd;
#if !defined(O_CLOEXEC) && !defined(_O_NOINHERIT) && !defined(O_NOINHERIT) && defined(F_GETFD) && defined(F_SETFD) &&  \
    defined(FD_CLOEXEC)
    int flags, saved;
#endif

    m = 0;
    switch (mode & 3)
    {
    case OREAD:
    case OEXEC:
        m = O_RDONLY;
        break;
    case OWRITE:
        m = O_WRONLY;
        break;
    case ORDWR:
        m = O_RDWR;
        break;
    }
    m |= O_CREAT | O_TRUNC;
    if (mode & OEXCL)
    {
        m |= O_EXCL;
    }

    if (perm & DMDIR)
    {
        if (mkdir(f, perm & 0777) < 0)
        {
            return -1;
        }
        perm &= ~DMDIR;
        m &= 3;
    }
    if (mode & OCEXEC)
    {
#ifdef O_CLOEXEC
        m |= O_CLOEXEC;
#elif defined(_O_NOINHERIT)
        m |= _O_NOINHERIT;
#elif defined(O_NOINHERIT)
        m |= O_NOINHERIT;
#endif
    }
    fd = open(f, m, perm);
#if !defined(O_CLOEXEC) && !defined(_O_NOINHERIT) && !defined(O_NOINHERIT) && defined(F_GETFD) && defined(F_SETFD) &&  \
    defined(FD_CLOEXEC)
    if (fd >= 0 && (mode & OCEXEC))
    {
        flags = fcntl(fd, F_GETFD);
        if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0)
        {
            saved = errno;
            close(fd);
            errno = saved;
            return -1;
        }
    }
#endif
    return fd;
}
