/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

{

    Fid *new;

    new = newfid(old->client, fid, old->qid);
    return new;
}

/* [standalone-contract:devwalk]
 * EN: devwalk: original server session/fid dispatch and callback-backed namespace operations.
 * RU: devwalk: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * file (Styxfile *file): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * fp (Fid *fp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nfp (Fid *nfp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (char **name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * nname (int nname): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * err (char **err): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Walkqid *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Walkqid *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Walkqid *devwalk(Client *c, Styxfile *file, Fid *fp, Fid *nfp, char **name, int nname, char **err)
{
    Styxserver *server;
    long j;
    Walkqid *wq;
    char *n;
    Styxfile *p, *f;
    Styxops *ops;
    Qid qid;

    *err = nil;
    server = c->server;
    ops = server->ops;

    wq = styxmalloc(sizeof(Walkqid) + (nname - 1) * sizeof(Qid));
    wq->nqid = 0;

    p = file;
    qid = p != nil ? p->d.qid : fp->qid;
    for (j = 0; j < nname; j++)
    {
        if (!(qid.type & QTDIR))
        {
            *err = Enotdir;
            goto Done;
        }
        if (p != nil && !styxperm(p, c->uname, OEXEC))
        {
            *err = Eperm;
            goto Done;
        }
        n = name[j];
        if (strcmp(n, ".") == 0)
        {
        Accept:
            wq->qid[wq->nqid++] = nfp->qid;
            continue;
        }
        if (p != nil && strcmp(n, "..") == 0 && p->parent)
        {
            decref(p);
            nfp->qid.path = p->parent->d.qid.path;
            nfp->qid.type = p->parent->d.qid.type;
            nfp->qid.vers = 0;
            incref(p->parent);
            p = p->parent;
            qid = p->d.qid;
            goto Accept;
        }

        if (ops->walk != nil)
        {
            char *e;

            e = ops->walk(&qid, n);
            if (e == nil)
            {
                decreff(nfp);
                nfp->qid = qid;
                increff(nfp);
                p = styxfindfile(server, qid.path);
                if (server->needfile && p == nil)
                {
                    goto Done;
                }
                qid = p != nil ? p->d.qid : nfp->qid;
                goto Accept;
            }
            if (!server->needfile)
            {
                *err = e;
                goto Done;
            }
        }

        if (p != nil)
        {
            for (f = p->child; f != nil; f = f->sibling)
            {
                if (strcmp(n, f->d.name) == 0)
                {
                    decref(p);
                    nfp->qid.path = f->d.qid.path;
                    nfp->qid.type = f->d.qid.type;
                    nfp->qid.vers = 0;
                    incref(f);
                    p = f;
                    qid = p->d.qid;
                    goto Accept;
                }
            }
        }
        if (j == 0 && *err == nil)
        {
            *err = Enonexist;
        }
        goto Done;
    }
Done:
    if (*err != nil)
    {
        styxfree(wq);
        return nil;
    }
    return wq;
}

/* Emit whole live directory records; an undersized buffer must not look like EOF. */


/* [standalone-contract:devdirread]
 * EN: devdirread: original server session/fid dispatch and callback-backed namespace operations.
 * RU: devdirread: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * fp (Fid *fp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * file (Styxfile *file): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * d (char *d): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: long; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static long devdirread(Fid *fp, Styxfile *file, char *d, long n)
{
    long dsz, m;
    Styxfile *f;
    int i;

    f = file->child;
    for (i = 0; i < fp->dri && f != nil; i++)
    {
        f = f->sibling;
    }
    for (m = 0; m < n && f != nil; fp->dri++)
    {
        dsz = sizeD2M(&f->d);
        if (dsz > n - m)
        {
            return m != 0 ? m : -1;
        }
        if (convD2M(&f->d, (uchar *)d, n - m) != dsz)
        {
            return -1;
        }
        m += dsz;
        d += dsz;
        f = f->sibling;
    }
    return m;
}

/* Provider directory data must contain only complete stat records. */


/* [standalone-contract:validdir]
 * EN: validdir: original server session/fid dispatch and callback-backed namespace operations.
 * RU: validdir: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * data (char *data): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (ulong n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int validdir(char *data, ulong n)
{
    uint size;
    uchar *p;

    p = (uchar *)data;
    while (n != 0)
    {
        if (n < BIT16SZ)
        {
            return 0;
        }
        size = GBIT16(p) + BIT16SZ;
        if (size > n || statcheck(p, size) < 0)
        {
            return 0;
        }
        p += size;
        n -= size;
    }
    return 1;
}

/* [standalone-contract:nilconv]
 * EN: nilconv: original server session/fid dispatch and callback-backed namespace operations.
 * RU: nilconv: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *nilconv(char *s)
{
    if (s != nil && s[0] == '\0')
    {
        return nil;
    }
    return s;
}

/* [standalone-contract:newfile]
 * EN: newfile: original server session/fid dispatch and callback-backed namespace operations.
 * RU: newfile: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * parent (Styxfile *parent): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * isdir (int isdir): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * qid (Path qid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * owner (char *owner): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Styxfile *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Styxfile *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Styxfile *newfile(Styxserver *server, Styxfile *parent, int isdir, Path qid, char *name, int mode, char *owner)
{
    Styxfile *file;
    Dir d;
    int h;

    if (qid == -1)
    {
        qid = server->qidgen++;
    }
    file = styxfindfile(server, qid);
    if (file != nil)
    {
        return nil;
    }
    if (parent != nil)
    {
        for (file = parent->child; file != nil; file = file->sibling)
        {
            if (strcmp(name, file->d.name) == 0)
            {
                return nil;
            }
        }
    }
    file = (Styxfile *)styxmalloc(sizeof(Styxfile));
    file->parent = parent;
    file->child = nil;
    h = hash(qid);
    file->next = server->ftab[h];
    server->ftab[h] = file;
    if (parent)
    {
        file->sibling = parent->child;
        parent->child = file;
    }
    else
    {
        file->sibling = nil;
    }

    d.type = 'X';
    d.dev = 'x';
    d.qid.path = qid;
    d.qid.type = 0;
    d.qid.vers = 0;
    d.mode = mode;
    d.atime = time(0);
    d.mtime = boottime;
    d.length = 0;
    d.name = strdup(name);
    d.uid = strdup(owner);
    d.gid = strdup(eve);
    d.muid = "";

    if (isdir)
    {
        d.qid.type |= QTDIR;
        d.mode |= DMDIR;
    }
    else
    {
        d.qid.type &= ~QTDIR;
        d.mode &= ~DMDIR;
    }

    file->d = d;
    file->ref = 0;
    file->open = 0;
    if (Debug)
    {
        fprint(2, "New file %s Qid=%llx\n", name, qid);
    }
    return file;
}

#include "attach.c"
#include "read.c"

/* [standalone-contract:dispatch]
 * EN: dispatch: original server session/fid dispatch and callback-backed namespace operations.
 * RU: dispatch: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void dispatch(Client *c)
{
    Fcall f;
    Fid *fp, *nfp;
    int i, open, mode, isdir;
    long dircount;
    ulong count;
    char ebuf[EMSGLEN];
    Walkqid *wq;
    Styxfile *file;
    Dir dir;
    Qid qid;
    Styxops *ops;
    char strs[MSGMAX + 4];

    ebuf[0] = 0;
    if (rd(c, &f) <= 0)
    {
        return;
    }
    if (f.type != Tversion && c->versioned < 0)
    {
        f.type = Rerror;
        f.ename = "version not negotiated";
        wr(c, &f);
        return;
    }
    if (f.type != Tversion && ((c->attach != nil && c->attach->tag == f.tag) || readtag(c, f.tag)))
    {
        c->state |= CDISC;
        return;
    }
    if (f.type == Tflush)
    {
        if (c->attach != nil && c->attach->tag == f.oldtag)
        {
            cancelattach(c);
        }
        flushread(c, f.oldtag);
        f.type = Rflush;
        wr(c, &f);
        return;
    }
    ops = c->server->ops;
    file = nil;
    fp = findfid(c, f.fid);
    c->current = fp;
    if (f.type != Tversion && f.type != Tauth && f.type != Tattach)
    {
        if (fp == nil)
        {
            f.type = Rerror;
            f.ename = Enofid;
            wr(c, &f);
            return;
        }
        else
        {
            file = styxfindfile(c->server, fp->qid.path);
            if (c->server->needfile && file == nil)
            {
                f.type = Rerror;
                f.ename = Enonexist;
                wr(c, &f);
                return;
            }
        }
    }
    /* if(fp == nil) fprint(2, "fid not found for %d\n", f.fid); */
    switch (f.type)
    {
    case Twalk:
        if (Debug)
        {
            fprint(2, "Twalk %d %d", f.fid, f.newfid);
            for (i = 0; i < f.nwname; i++)
