/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 convM2D support.
 * RU: исходная POSIX-поддержка lib9 convM2D.
 */
#include "lib9.h"
#include "fcall.h"

/* Validate every counted string before advancing beyond the available record. */


/* [standalone-contract:statcheck]
 * EN: Validate a complete encoded stat record and each counted string without exposing a partial record.
 * RU: Проверить целую stat-запись и каждую counted string, не выдавая частично разобранную запись.
 * buf (uchar *buf): Counted stat record to validate structurally.
 * RU: Counted stat-запись для структурной проверки.
 * nbuf (uint nbuf): Accessible extent; must contain the complete counted record.
 * RU: Доступная длина; должна содержать целую counted запись.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int statcheck(uchar *buf, uint nbuf)
{
    uint left, n;
    int i;

    if (buf == nil || nbuf < STATFIXLEN || nbuf != BIT16SZ + GBIT16(buf))
    {
        return -1;
    }
    left = nbuf - (STATFIXLEN - 4 * BIT16SZ);
    buf += STATFIXLEN - 4 * BIT16SZ;
    for (i = 0; i < 4; i++)
    {
        if (left < BIT16SZ)
        {
            return -1;
        }
        n = GBIT16(buf);
        if (n > left - BIT16SZ)
        {
            return -1;
        }
        buf += BIT16SZ + n;
        left -= BIT16SZ + n;
    }
    return left == 0 ? 0 : -1;
}

static char nullstring[] = "";

/* Decode only the declared first record, leaving outputs untouched on bad input. */


/* [standalone-contract:convM2D]
 * EN: Decode a validated standard Dir record and copy its strings into caller-supplied storage when provided.
 * RU: Декодировать проверенную Dir-запись и скопировать строки в буфер вызывающего, если он задан.
 * buf (uchar *buf): Input counted stat bytes; no native Dir layout is accepted.
 * RU: Входные counted stat-байты; native layout Dir не принимается.
 * nbuf (uint nbuf): Accessible input extent in bytes.
 * RU: Доступная входная длина в байтах.
 * d (Dir *d): Caller-owned decoded scalar fields and string pointers.
 * RU: Приёмник декодированных scalar fields и указателей строк.
 * strs (char *strs): Caller-owned decoded-string storage, or NULL for empty string fields. nbuf bytes suffice for a valid record.
 * RU: Буфер декодированных строк либо NULL для пустых string fields. Для допустимой записи достаточно nbuf байт.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convM2D(uchar *buf, uint nbuf, Dir *d, char *strs)
{
    uchar *p, *ebuf;
    char *sv[4];
    int i, ns;
    uint size;

    if (buf == nil || d == nil || nbuf < STATFIXLEN)
    {
        return 0;
    }
    size = BIT16SZ + GBIT16(buf);
    if (size > nbuf || statcheck(buf, size) < 0)
    {
        return 0;
    }
    nbuf = size;

    p = buf;
    ebuf = buf + nbuf;

    p += BIT16SZ; /* size was checked before any destination writes */
    d->type = GBIT16(p);
    p += BIT16SZ;
    d->dev = GBIT32(p);
    p += BIT32SZ;
    d->qid.type = GBIT8(p);
    p += BIT8SZ;
    d->qid.vers = GBIT32(p);
    p += BIT32SZ;
    d->qid.path = GBIT64(p);
    p += BIT64SZ;
    d->mode = GBIT32(p);
    p += BIT32SZ;
    d->atime = GBIT32(p);
    p += BIT32SZ;
    d->mtime = GBIT32(p);
    p += BIT32SZ;
    d->length = GBIT64(p);
    p += BIT64SZ;

    for (i = 0; i < 4; i++)
    {
        if (ebuf - p < BIT16SZ)
        {
            return 0;
        }
        ns = GBIT16(p);
        p += BIT16SZ;
        if (ns > ebuf - p)
        {
            return 0;
        }
        if (strs)
        {
            sv[i] = strs;
            memmove(strs, p, ns);
            strs += ns;
            *strs++ = '\0';
        }
        p += ns;
    }

    if (strs)
    {
        d->name = sv[0];
        d->uid = sv[1];
        d->gid = sv[2];
        d->muid = sv[3];
    }
    else
    {
        d->name = nullstring;
        d->uid = nullstring;
        d->gid = nullstring;
        d->muid = nullstring;
    }

    return p - buf;
}
