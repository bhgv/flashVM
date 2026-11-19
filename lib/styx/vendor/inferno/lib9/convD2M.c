/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 convD2M support.
 * RU: исходная POSIX-поддержка lib9 convD2M.
 */
#include "lib9.h"
#include "fcall.h"

/* Bound the whole stat entry before any 16-bit wire count is written. */


/* [standalone-contract:statsz]
 * EN: statsz: original POSIX lib9 convD2M support.
 * RU: statsz: исходная POSIX-поддержка lib9 convD2M.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * nsv (uint nsv[4]): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uint statsz(Dir *d, uint nsv[4])
{
    char *sv[4];
    uint n, ns, left;
    int i;

    sv[0] = d->name;
    sv[1] = d->uid;
    sv[2] = d->gid;
    sv[3] = d->muid;

    ns = STATFIXLEN;
    for (i = 0; i < 4; i++)
    {
        left = 0xffff + BIT16SZ - ns;
        n = 0;
        if (sv[i] != nil)
        {
            while (n <= left && sv[i][n] != '\0')
            {
                n++;
            }
            if (n > left)
            {
                return 0;
            }
        }
        nsv[i] = n;
        ns += n;
    }
    return ns;
}

/* Report zero when the stat size cannot be represented on the wire. */


/* [standalone-contract:sizeD2M]
 * EN: Calculate the complete standard stat-record size, including its leading size field.
 * RU: Вычислить полный размер стандартной stat-записи, включая начальное поле длины.
 * d (Dir *d): Input directory metadata; strings must be valid NUL-terminated C strings.
 * RU: Входные metadata каталога; строки должны быть допустимыми C-строками с NUL.
 * Return: uint; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint sizeD2M(Dir *d)
{
    uint nsv[4];

    return statsz(d, nsv);
}

/* Preserve short-buffer size reporting only for representable entries. */


/* [standalone-contract:convD2M]
 * EN: Encode one standard Dir record; a short buffer can receive only its two-byte required-size prefix.
 * RU: Кодировать стандартную Dir-запись; короткий буфер может получить только двухбайтовый prefix требуемой длины.
 * d (Dir *d): Input directory metadata to encode as one counted stat record.
 * RU: Входные metadata для одной counted stat-записи.
 * buf (uchar *buf): Caller-owned writable destination; do not confuse a two-byte size prefix with a complete record.
 * RU: Выходной буфер вызывающего; двухбайтовый size prefix не является целой записью.
 * nbuf (uint nbuf): Destination capacity in bytes, including the leading two-byte length.
 * RU: Ёмкость приёмника в байтах, включая двухбайтовую длину.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convD2M(Dir *d, uchar *buf, uint nbuf)
{
    uchar *p;
    char *sv[4];
    int i;
    uint ns, nsv[4], ss;

    if (nbuf < BIT16SZ)
    {
        return 0;
    }

    ss = statsz(d, nsv);
    if (ss == 0)
    {
        return 0;
    }
    p = buf;

    sv[0] = d->name;
    sv[1] = d->uid;
    sv[2] = d->gid;
    sv[3] = d->muid;

    /* Set size before returning a short buffer, so the caller can retry. */
    /* note that length excludes count field itself */
    PBIT16(p, ss - BIT16SZ);
    p += BIT16SZ;

    if (ss > nbuf)
    {
        return BIT16SZ;
    }

    PBIT16(p, d->type);
    p += BIT16SZ;
    PBIT32(p, d->dev);
    p += BIT32SZ;
    PBIT8(p, d->qid.type);
    p += BIT8SZ;
    PBIT32(p, d->qid.vers);
    p += BIT32SZ;
    PBIT64(p, d->qid.path);
    p += BIT64SZ;
    PBIT32(p, d->mode);
    p += BIT32SZ;
    PBIT32(p, d->atime);
    p += BIT32SZ;
    PBIT32(p, d->mtime);
    p += BIT32SZ;
    PBIT64(p, d->length);
    p += BIT64SZ;

    for (i = 0; i < 4; i++)
    {
        ns = nsv[i];
        PBIT16(p, ns);
        p += BIT16SZ;
        if (ns)
        {
            memmove(p, sv[i], ns);
        }
        p += ns;
    }

    if (ss != p - buf)
    {
        return 0;
    }

    return p - buf;
}
