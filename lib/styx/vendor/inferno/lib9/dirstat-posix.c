/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 dirstat-posix support.
 * RU: исходная POSIX-поддержка lib9 dirstat-posix.
 */
#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

static char nullstring[] = "";
static char Enovmem[] = "out of memory";

/* Store every variable string in the returned Dir's single owned allocation. */


/* [standalone-contract:statconv]
 * EN: statconv: original POSIX lib9 dirstat-posix support.
 * RU: statconv: исходная POSIX-поддержка lib9 dirstat-posix.
 * s (struct stat *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * nname (ulong nname): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Dir *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Dir *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Dir *statconv(struct stat *s, char *name, ulong nname)
{
    struct passwd *p;
    struct group *g;
    Dir *dir;
    ulong str, nuid, ngid, limit;
    char *n;

    limit = 0x7fffffff - sizeof(Dir);
    if (nname >= limit)
    {
        goto Nomem;
    }
    str = nname + 1;
    p = getpwuid(s->st_uid);
    nuid = p == nil ? 0 : strlen(p->pw_name);
    g = getgrgid(s->st_gid);
    ngid = g == nil ? 0 : strlen(g->gr_name);
    if (p != nil)
    {
        if (nuid >= limit - str)
        {
            goto Nomem;
        }
        str += nuid + 1;
    }
    if (g != nil)
    {
        if (ngid >= limit - str)
        {
            goto Nomem;
        }
        str += ngid + 1;
    }
    dir = malloc(sizeof(Dir) + str);
    if (dir == nil)
    {
        goto Nomem;
    }
    n = (char *)(dir + 1);
    dir->name = n;
    memmove(n, name, nname);
    n[nname] = '\0';
    n += nname + 1;
    dir->uid = dir->gid = dir->muid = nullstring;
    if (p != nil)
    {
        dir->uid = n;
        memmove(n, p->pw_name, nuid + 1);
        n += nuid + 1;
    }
    if (g != nil)
    {
        dir->gid = n;
        memmove(n, g->gr_name, ngid + 1);
    }
    dir->qid.type = S_ISDIR(s->st_mode) ? QTDIR : QTFILE;
    dir->qid.path = s->st_ino;
    dir->qid.vers = s->st_mtime;
    dir->mode = ((ulong)dir->qid.type << 24) | (s->st_mode & 0777);
    dir->atime = s->st_atime;
    dir->mtime = s->st_mtime;
    dir->length = s->st_size;
    dir->dev = s->st_dev;
    dir->type = S_ISFIFO(s->st_mode) ? '|' : 'M';
    return dir;

Nomem:
    werrstr(Enovmem);
    return nil;
}

/* Descriptor metadata has an empty, owned name when no path is available. */


/* [standalone-contract:dirfstat]
 * EN: dirfstat: original POSIX lib9 dirstat-posix support.
 * RU: dirfstat: исходная POSIX-поддержка lib9 dirstat-posix.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Dir *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Dir *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Dir *dirfstat(int fd)
{
    struct stat sbuf;

    if (fstat(fd, &sbuf) < 0)
    {
        return nil;
    }
    return statconv(&sbuf, nullstring, 0);
}

/* Copy the final path component, ignoring trailing separators except for root. */


/* [standalone-contract:dirstat]
 * EN: dirstat: original POSIX lib9 dirstat-posix support.
 * RU: dirstat: исходная POSIX-поддержка lib9 dirstat-posix.
 * f (char *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Dir *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Dir *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Dir *dirstat(char *f)
{
    struct stat sbuf;
    char *p, *end;

    if (stat(f, &sbuf) < 0)
    {
        return nil;
    }
    end = f + strlen(f);
    while (end > f + 1 && end[-1] == '/')
    {
        end--;
    }
    p = end;
    while (p > f && p[-1] != '/')
    {
        p--;
    }
    if (p == end && end == f + 1 && f[0] == '/')
    {
        p = f;
    }
    return statconv(&sbuf, p, end - p);
}
