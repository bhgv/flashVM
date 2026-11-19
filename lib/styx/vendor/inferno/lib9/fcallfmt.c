/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fcallfmt support.
 * RU: исходная POSIX-поддержка lib9 fcallfmt.
 */
/*
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */

#include <lib9.h>
#include "fcall.h"

static uint dumpsome(char *, char *, char *, long);
static void fdirconv(char *, char *, Dir *);
static char *qidtype(char *, uchar);

#define QIDFMT "(%.16llux %lud %s)"

/* [standalone-contract:fcallfmt]
 * EN: fcallfmt: original POSIX lib9 fcallfmt support.
 * RU: fcallfmt: исходная POSIX-поддержка lib9 fcallfmt.
 * fmt (Fmt *fmt): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fcallfmt(Fmt *fmt)
{
    Fcall *f;
    int fid, type, tag, i;
    char buf[512], tmp[200];
    char *p, *e;
    Dir *d;
    Qid *q;

    e = buf + sizeof(buf);
    f = va_arg(fmt->args, Fcall *);
    type = f->type;
    fid = f->fid;
    tag = f->tag;
    switch (type)
    {
    case Tversion: /* 100 */
        seprint(buf, e, "Tversion tag %ud msize %ud version '%s'", tag, f->msize, f->version);
        break;
    case Rversion:
        seprint(buf, e, "Rversion tag %ud msize %ud version '%s'", tag, f->msize, f->version);
        break;
    case Tauth: /* 102 */
        seprint(buf, e, "Tauth tag %ud afid %d uname %s aname %s", tag, f->afid, f->uname, f->aname);
        break;
    case Rauth:
        seprint(buf, e, "Rauth tag %ud qid " QIDFMT, tag, f->aqid.path, f->aqid.vers, qidtype(tmp, f->aqid.type));
        break;
    case Tattach: /* 104 */
        seprint(buf, e, "Tattach tag %ud fid %d afid %d uname %s aname %s", tag, fid, f->afid, f->uname, f->aname);
        break;
    case Rattach:
        seprint(buf, e, "Rattach tag %ud qid " QIDFMT, tag, f->qid.path, f->qid.vers, qidtype(tmp, f->qid.type));
        break;
    case Rerror: /* 107; 106 (Terror) illegal */
        seprint(buf, e, "Rerror tag %ud ename %s", tag, f->ename);
        break;
    case Tflush: /* 108 */
        seprint(buf, e, "Tflush tag %ud oldtag %ud", tag, f->oldtag);
        break;
    case Rflush:
        seprint(buf, e, "Rflush tag %ud", tag);
        break;
    case Twalk: /* 110 */
        p = seprint(buf, e, "Twalk tag %ud fid %d newfid %d nwname %d ", tag, fid, f->newfid, f->nwname);
        if (f->nwname <= MAXWELEM)
        {
            for (i = 0; i < f->nwname; i++)
            {
                p = seprint(p, e, "%d:%s ", i, f->wname[i]);
            }
        }
        break;
    case Rwalk:
        p = seprint(buf, e, "Rwalk tag %ud nwqid %ud ", tag, f->nwqid);
        if (f->nwqid <= MAXWELEM)
        {
            for (i = 0; i < f->nwqid; i++)
            {
                q = &f->wqid[i];
                p = seprint(p, e, "%d:" QIDFMT " ", i, q->path, q->vers, qidtype(tmp, q->type));
            }
        }
        break;
    case Topen: /* 112 */
        seprint(buf, e, "Topen tag %ud fid %ud mode %d", tag, fid, f->mode);
        break;
    case Ropen:
        seprint(buf,
                e,
                "Ropen tag %ud qid " QIDFMT " iounit %ud ",
                tag,
                f->qid.path,
                f->qid.vers,
                qidtype(tmp, f->qid.type),
                f->iounit);
        break;
    case Tcreate: /* 114 */
        seprint(buf, e, "Tcreate tag %ud fid %ud name %s perm %M mode %d", tag, fid, f->name, (ulong)f->perm, f->mode);
        break;
    case Rcreate:
        seprint(buf,
                e,
                "Rcreate tag %ud qid " QIDFMT " iounit %ud ",
                tag,
                f->qid.path,
                f->qid.vers,
                qidtype(tmp, f->qid.type),
                f->iounit);
        break;
    case Tread: /* 116 */
        seprint(buf, e, "Tread tag %ud fid %d offset %lld count %ud", tag, fid, f->offset, f->count);
        break;
    case Rread:
        p = seprint(buf, e, "Rread tag %ud count %ud ", tag, f->count);
        dumpsome(p, e, f->data, f->count);
        break;
    case Twrite: /* 118 */
        p = seprint(buf, e, "Twrite tag %ud fid %d offset %lld count %ud ", tag, fid, f->offset, f->count);
        dumpsome(p, e, f->data, f->count);
        break;
    case Rwrite:
        seprint(buf, e, "Rwrite tag %ud count %ud", tag, f->count);
        break;
    case Tclunk: /* 120 */
        seprint(buf, e, "Tclunk tag %ud fid %ud", tag, fid);
        break;
    case Rclunk:
        seprint(buf, e, "Rclunk tag %ud", tag);
        break;
    case Tremove: /* 122 */
        seprint(buf, e, "Tremove tag %ud fid %ud", tag, fid);
        break;
    case Rremove:
        seprint(buf, e, "Rremove tag %ud", tag);
        break;
    case Tstat: /* 124 */
        seprint(buf, e, "Tstat tag %ud fid %ud", tag, fid);
        break;
    case Rstat:
        p = seprint(buf, e, "Rstat tag %ud ", tag);
        if (f->nstat > sizeof tmp)
        {
            seprint(p, e, " stat(%d bytes)", f->nstat);
        }
        else
        {
            d = (Dir *)tmp;
            convM2D(f->stat, f->nstat, d, (char *)(d + 1));
            seprint(p, e, " stat ");
            fdirconv(p + 6, e, d);
        }
        break;
    case Twstat: /* 126 */
        p = seprint(buf, e, "Twstat tag %ud fid %ud", tag, fid);
        if (f->nstat > sizeof tmp)
        {
            seprint(p, e, " stat(%d bytes)", f->nstat);
        }
        else
        {
            d = (Dir *)tmp;
            convM2D(f->stat, f->nstat, d, (char *)(d + 1));
            seprint(p, e, " stat ");
            fdirconv(p + 6, e, d);
        }
        break;
    case Rwstat:
        seprint(buf, e, "Rwstat tag %ud", tag);
        break;
    default:
        seprint(buf, e, "unknown type %d", type);
    }
    return fmtstrcpy(fmt, buf);
}

/* [standalone-contract:qidtype]
 * EN: qidtype: original POSIX lib9 fcallfmt support.
 * RU: qidtype: исходная POSIX-поддержка lib9 fcallfmt.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * t (uchar t): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *qidtype(char *s, uchar t)
{
    char *p;

    p = s;
    if (t & QTDIR)
    {
        *p++ = 'd';
    }
    if (t & QTAPPEND)
    {
        *p++ = 'a';
    }
    if (t & QTEXCL)
    {
        *p++ = 'l';
    }
    if (t & QTAUTH)
    {
        *p++ = 'A';
    }
    *p = '\0';
    return s;
}

/* [standalone-contract:dirfmt]
 * EN: dirfmt: original POSIX lib9 fcallfmt support.
 * RU: dirfmt: исходная POSIX-поддержка lib9 fcallfmt.
 * fmt (Fmt *fmt): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dirfmt(Fmt *fmt)
{
    char buf[160];

    fdirconv(buf, buf + sizeof buf, va_arg(fmt->args, Dir *));
    return fmtstrcpy(fmt, buf);
}

/* [standalone-contract:fdirconv]
 * EN: fdirconv: original POSIX lib9 fcallfmt support.
 * RU: fdirconv: исходная POSIX-поддержка lib9 fcallfmt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * e (char *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void fdirconv(char *buf, char *e, Dir *d)
{
    char tmp[16];

    seprint(buf,
            e,
            "'%s' '%s' '%s' '%s' "
            "q " QIDFMT " m %#luo "
            "at %ld mt %ld l %lld "
            "t %d d %d",
            d->name,
            d->uid,
            d->gid,
            d->muid,
            d->qid.path,
            d->qid.vers,
            qidtype(tmp, d->qid.type),
            d->mode,
            d->atime,
            d->mtime,
            d->length,
            d->type,
            d->dev);
}

/*
 * dump out count (or DUMPL, if count is bigger) bytes from
 * buf to ans, as a string if they are all printable,
 * else as a series of hex bytes
 */
#define DUMPL 64

/* [standalone-contract:dumpsome]
 * EN: dumpsome: original POSIX lib9 fcallfmt support.
 * RU: dumpsome: исходная POSIX-поддержка lib9 fcallfmt.
 * ans (char *ans): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * e (char *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * count (long count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: uint; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uint; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static uint dumpsome(char *ans, char *e, char *buf, long count)
{
    int i, printable;
    char *p;

    if (buf == nil)
    {
        seprint(ans, e, "<no data>");
        return strlen(ans);
    }
    printable = 1;
    if (count > DUMPL)
    {
        count = DUMPL;
    }
    for (i = 0; i < count && printable; i++)
    {
        if ((buf[i] < 32 && buf[i] != '\n' && buf[i] != '\t') || (uchar)buf[i] > 127)
        {
            printable = 0;
        }
    }
    p = ans;
    *p++ = '\'';
    if (printable)
    {
        if (count > e - p - 2)
        {
            count = e - p - 2;
        }
        memmove(p, buf, count);
        p += count;
    }
    else
    {
        if (2 * count > e - p - 2)
        {
            count = (e - p - 2) / 2;
        }
        for (i = 0; i < count; i++)
        {
            if (i > 0 && i % 4 == 0)
            {
                *p++ = ' ';
            }
            sprint(p, "%2.2ux", buf[i]);
            p += 2;
        }
    }
    *p++ = '\'';
    *p = 0;
    return p - ans;
}
