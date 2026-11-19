/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 dirstat-Nt support.
 * RU: исходная POSIX-поддержка lib9 dirstat-Nt.
 */
#include "lib9.h"
#include <sys/types.h>
#include <sys/stat.h>

#define ISTYPE(s, t) (((s)->st_mode & S_IFMT) == t)

static char nullstring[] = "";
static char Enovmem[] = "out of memory";

/* Keep the basename in the returned Dir allocation, independently of its caller. */


/* [standalone-contract:statconv]
 * EN: statconv: original POSIX lib9 dirstat-Nt support.
 * RU: statconv: исходная POSIX-поддержка lib9 dirstat-Nt.
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
    Dir *dir;

#ifdef NO
    extern char *GetNameFromID(int);

        uid = GetNameFromID(s->st_uid), NAMELEN);
        gid = GetNameFromID(s->st_gid), NAMELEN);
#endif
        if (nname > 0x7fffffff - sizeof(Dir) - 1)
        {
            werrstr(Enovmem);
            return nil;
        }
        dir = malloc(sizeof(Dir) + nname + 1);
        if (dir == nil)
        {
            werrstr(Enovmem);
            return nil;
        }
        dir->name = (char *)(dir + 1);
        memmove(dir->name, name, nname);
        dir->name[nname] = '\0';
        dir->uid = dir->gid = dir->muid = nullstring;
        dir->qid.type = ISTYPE(s, _S_IFDIR) ? QTDIR : QTFILE;
        dir->qid.path = s->st_ino;
        dir->qid.vers = s->st_mtime;
        dir->mode = ((ulong)dir->qid.type << 24) | (s->st_mode & 0777);
        dir->atime = s->st_atime;
        dir->mtime = s->st_mtime;
        dir->length = s->st_size;
        dir->dev = s->st_dev;
        dir->type = ISTYPE(s, _S_IFIFO) ? '|' : 'M';
        return dir;
}

/* A descriptor stat owns an empty name when no path is available. */


/* [standalone-contract:dirfstat]
 * EN: dirfstat: original POSIX lib9 dirstat-Nt support.
 * RU: dirfstat: исходная POSIX-поддержка lib9 dirstat-Nt.
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
 * EN: dirstat: original POSIX lib9 dirstat-Nt support.
 * RU: dirstat: исходная POSIX-поддержка lib9 dirstat-Nt.
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
