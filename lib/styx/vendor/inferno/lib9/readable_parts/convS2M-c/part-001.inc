/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of convS2M.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 convS2M support.
 * RU: исходная POSIX-поддержка lib9 convS2M.
 */
#include "lib9.h"
#include "fcall.h"

/* Encode a string whose 16-bit length was validated by sizeS2M. */


/* [standalone-contract:pstring]
 * EN: pstring: original POSIX lib9 convS2M support.
 * RU: pstring: исходная POSIX-поддержка lib9 convS2M.
 * p (uchar *p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: uchar *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uchar *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uchar *pstring(uchar *p, char *s)
{
    uint n;

    if (s == nil)
    {
        PBIT16(p, 0);
        p += BIT16SZ;
        return p;
    }

    n = strlen(s);
    PBIT16(p, n);
    p += BIT16SZ;
    memmove(p, s, n);
    p += n;
    return p;
}

/* Encode a qid in its fixed-width wire representation. */


/* [standalone-contract:pqid]
 * EN: pqid: original POSIX lib9 convS2M support.
 * RU: pqid: исходная POSIX-поддержка lib9 convS2M.
 * p (uchar *p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * q (Qid *q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * Return: uchar *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uchar *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uchar *pqid(uchar *p, Qid *q)
{
    PBIT8(p, q->type);
    p += BIT8SZ;
    PBIT32(p, q->vers);
    p += BIT32SZ;
    PBIT64(p, q->path);
    p += BIT64SZ;
    return p;
}

/* Return zero for a string that cannot fit its 16-bit wire count. */


/* [standalone-contract:stringsz]
 * EN: stringsz: original POSIX lib9 convS2M support.
 * RU: stringsz: исходная POSIX-поддержка lib9 convS2M.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uint stringsz(char *s)
{
    uint n;

    if (s == nil)
    {
        return BIT16SZ;
    }
    for (n = 0; n <= 0xffff; n++)
    {
        if (s[n] == '\0')
        {
            return BIT16SZ + n;
        }
    }
    return 0;
}

/* Validate variable fields before sizing or touching the output buffer. */


/* [standalone-contract:sizeS2M]
 * EN: Calculate the exact standard message size before encoding; reject unsupported or unrepresentable fields.
 * RU: Вычислить точный размер стандартного сообщения до кодирования; отклонить неподдерживаемые поля.
 * f (Fcall *f): Input message to measure, with valid referenced strings and payload.
 * RU: Входное сообщение для измерения с допустимыми строками и payload.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint sizeS2M(Fcall *f)
{
    uint n, ns;
    int i;

    n = 0;
    n += BIT32SZ; /* size */
    n += BIT8SZ;  /* type */
    n += BIT16SZ; /* tag */

    switch (f->type)
    {
    default:
        return 0;

    case Tversion:
        n += BIT32SZ;
        if ((ns = stringsz(f->version)) == 0)
        {
            return 0;
        }
        n += ns;
        break;

    case Tflush:
        n += BIT16SZ;
        break;

    case Tauth:
        n += BIT32SZ;
        if ((ns = stringsz(f->uname)) == 0)
        {
            return 0;
        }
        n += ns;
        if ((ns = stringsz(f->aname)) == 0)
        {
            return 0;
        }
        n += ns;
        break;

    case Tattach:
        n += BIT32SZ;
        n += BIT32SZ;
        if ((ns = stringsz(f->uname)) == 0)
        {
            return 0;
        }
        n += ns;
        if ((ns = stringsz(f->aname)) == 0)
        {
            return 0;
        }
        n += ns;
        break;

    case Twalk:
        if (f->nwname > MAXWELEM)
        {
            return 0;
        }
        n += BIT32SZ;
        n += BIT32SZ;
        n += BIT16SZ;
        for (i = 0; i < f->nwname; i++)
        {
            if ((ns = stringsz(f->wname[i])) == 0)
            {
                return 0;
            }
            n += ns;
        }
        break;

    case Topen:
        n += BIT32SZ;
        n += BIT8SZ;
        break;

    case Tcreate:
        n += BIT32SZ;
        if ((ns = stringsz(f->name)) == 0)
        {
            return 0;
        }
        n += ns;
        n += BIT32SZ;
        n += BIT8SZ;
        break;

    case Tread:
        n += BIT32SZ;
        n += BIT64SZ;
        n += BIT32SZ;
        break;

    case Twrite:
        n += BIT32SZ;
        n += BIT64SZ;
        n += BIT32SZ;
        if (f->count > (uint)~0 - n)
        {
            return 0;
        }
        n += f->count;
        break;

    case Tclunk:
    case Tremove:
        n += BIT32SZ;
        break;

    case Tstat:
        n += BIT32SZ;
        break;

    case Twstat:
        n += BIT32SZ;
        n += BIT16SZ;
        n += f->nstat;
        break;
        /*
 */

    case Rversion:
        n += BIT32SZ;
        if ((ns = stringsz(f->version)) == 0)
        {
            return 0;
        }
        n += ns;
        break;

    case Rerror:
        if ((ns = stringsz(f->ename)) == 0)
        {
            return 0;
        }
        n += ns;
        break;

    case Rflush:
        break;

    case Rauth:
        n += QIDSZ;
        break;

    case Rattach:
        n += QIDSZ;
        break;

    case Rwalk:
        if (f->nwqid > MAXWELEM)
        {
            return 0;
        }
        n += BIT16SZ;
        n += f->nwqid * QIDSZ;
        break;

    case Ropen:
    case Rcreate:
        n += QIDSZ;
        n += BIT32SZ;
        break;

    case Rread:
        n += BIT32SZ;
        if (f->count > (uint)~0 - n)
        {
            return 0;
        }
        n += f->count;
        break;

    case Rwrite:
        n += BIT32SZ;
        break;

    case Rclunk:
        break;

    case Rremove:
        break;

    case Rstat:
        n += BIT16SZ;
        n += f->nstat;
        break;

    case Rwstat:
        break;
    }
    return n;
}

/* Serialize only messages whose complete wire size has been validated. */


/* [standalone-contract:convS2M]
 * EN: Encode a native Fcall as standard 9P2000 bytes with explicit little-endian fields.
 * RU: Кодировать native Fcall в стандартные 9P2000 байты с явным little-endian полей.
 * f (Fcall *f): Input message fields and borrowed strings/data to encode; caller retains ownership.
 * RU: Входные поля и заимствованные строки/data; владение остаётся у вызывающего.
 * ap (uchar *ap): Caller-owned output storage, disjoint from referenced input fields.
 * RU: Выходной буфер вызывающего, не перекрывающий входные поля.
 * nap (uint nap): Available output bytes; sizeS2M reports the required complete extent.
 * RU: Доступная выходная ёмкость; sizeS2M сообщает размер целого сообщения.
 * Return: uint; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uint convS2M(Fcall *f, uchar *ap, uint nap)
{
    uchar *p;
    uint i, size;

    size = sizeS2M(f);
    if (size == 0)
    {
        return 0;
    }
    if (size > nap)
    {
        return 0;
    }
    if ((f->type == Twrite || f->type == Rread) && f->count != 0 && f->data == nil)
    {
        return 0;
    }
    if ((f->type == Twstat || f->type == Rstat) && f->nstat != 0 && f->stat == nil)
    {
        return 0;
    }

    p = (uchar *)ap;

    PBIT32(p, size);
    p += BIT32SZ;
    PBIT8(p, f->type);
    p += BIT8SZ;
    PBIT16(p, f->tag);
    p += BIT16SZ;

    switch (f->type)
    {
    default:
        return 0;

    case Tversion:
        PBIT32(p, f->msize);
        p += BIT32SZ;
        p = pstring(p, f->version);
        break;

    case Tflush:
        PBIT16(p, f->oldtag);
        p += BIT16SZ;
        break;

    case Tauth:
        PBIT32(p, f->afid);
        p += BIT32SZ;
        p = pstring(p, f->uname);
        p = pstring(p, f->aname);
        break;

    case Tattach:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT32(p, f->afid);
        p += BIT32SZ;
        p = pstring(p, f->uname);
        p = pstring(p, f->aname);
        break;

    case Twalk:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT32(p, f->newfid);
        p += BIT32SZ;
        PBIT16(p, f->nwname);
        p += BIT16SZ;
        if (f->nwname > MAXWELEM)
        {
            return 0;
        }
        for (i = 0; i < f->nwname; i++)
        {
            p = pstring(p, f->wname[i]);
        }
        break;

    case Topen:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT8(p, f->mode);
        p += BIT8SZ;
        break;

    case Tcreate:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        p = pstring(p, f->name);
        PBIT32(p, f->perm);
        p += BIT32SZ;
        PBIT8(p, f->mode);
        p += BIT8SZ;
        break;

    case Tread:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT64(p, f->offset);
        p += BIT64SZ;
        PBIT32(p, f->count);
