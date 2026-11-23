/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

        {
            next = s->sibling;
            sremove(server, s);
        }
    }
    for (p = &server->ftab[hash(f->d.qid.path)]; *p; p = &(*p)->next)
    {
        if (*p == f)
        {
            *p = f->next;
            break;
        }
    }
    if (f->parent != nil)
    {
        for (p = &f->parent->child; *p; p = &(*p)->sibling)
        {
            if (*p == f)
            {
                *p = f->sibling;
                break;
            }
        }
    }
    styxfree(f->d.name);
    styxfree(f->d.uid);
    styxfree(f->d.gid);
    styxfree(f);
}

/* [standalone-contract:styxrmfile]
 * EN: styxrmfile: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxrmfile: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * qid (Path qid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrmfile(Styxserver *server, Path qid)
{
    Styxfile *f;

    f = styxfindfile(server, qid);
    if (f != nil)
    {
        if (f->parent == nil)
        {
            return -1;
        }
        sremove(server, f);
        return 0;
    }
    return -1;
}

/* [standalone-contract:incref]
 * EN: incref: original server session/fid dispatch and callback-backed namespace operations.
 * RU: incref: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Styxfile *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void incref(Styxfile *f)
{
    if (f != nil)
    {
        f->ref++;
    }
}

/* [standalone-contract:decref]
 * EN: decref: original server session/fid dispatch and callback-backed namespace operations.
 * RU: decref: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Styxfile *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void decref(Styxfile *f)
{
    if (f != nil)
    {
        --f->ref;
    }
}

/* [standalone-contract:increff]
 * EN: increff: original server session/fid dispatch and callback-backed namespace operations.
 * RU: increff: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Fid *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void increff(Fid *f)
{
    incref(styxfindfile(f->client->server, f->qid.path));
}

/* [standalone-contract:decreff]
 * EN: decreff: original server session/fid dispatch and callback-backed namespace operations.
 * RU: decreff: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Fid *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void decreff(Fid *f)
{
    decref(styxfindfile(f->client->server, f->qid.path));
}

/* [standalone-contract:incopen]
 * EN: incopen: original server session/fid dispatch and callback-backed namespace operations.
 * RU: incopen: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Fid *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void incopen(Fid *f)
{
    Styxfile *file;

    if (f->open && (file = styxfindfile(f->client->server, f->qid.path)) != nil)
    {
        file->open++;
    }
}

/* [standalone-contract:decopen]
 * EN: decopen: original server session/fid dispatch and callback-backed namespace operations.
 * RU: decopen: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Fid *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void decopen(Fid *f)
{
    Styxfile *file;

    if (f->open && (file = styxfindfile(f->client->server, f->qid.path)) != nil)
    {
        file->open--;
    }
}

/* [standalone-contract:styxperm]
 * EN: styxperm: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxperm: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * f (Styxfile *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * uid (char *uid): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxperm(Styxfile *f, char *uid, int mode)
{
    int m, p;

    p = 0;
    switch (mode & 3)
    {
    case OREAD:
        p = AREAD;
        break;
    case OWRITE:
        p = AWRITE;
        break;
    case ORDWR:
        p = AREAD + AWRITE;
        break;
    case OEXEC:
        p = AEXEC;
        break;
    }
    if (mode & OTRUNC)
    {
        p |= AWRITE;
    }
    m = f->d.mode & 7;
    if ((p & m) == p)
    {
        return 1;
    }
    if (strcmp(f->d.uid, uid) == 0)
    {
        m |= (f->d.mode >> 6) & 7;
        if ((p & m) == p)
        {
            return 1;
        }
    }
    if (strcmp(f->d.gid, uid) == 0)
    {
        m |= (f->d.mode >> 3) & 7;
        if ((p & m) == p)
        {
            return 1;
        }
    }
    return 0;
}

/* [standalone-contract:hash]
 * EN: hash: original server session/fid dispatch and callback-backed namespace operations.
 * RU: hash: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * path (Path path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int hash(Path path)
{
    return path & (TABSZ - 1);
}

/* [standalone-contract:styxfindfile]
 * EN: styxfindfile: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxfindfile: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * path (Path path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * Return: Styxfile *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Styxfile *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Styxfile *styxfindfile(Styxserver *server, Path path)
{
    Styxfile *f;

    for (f = server->ftab[hash(path)]; f != nil; f = f->next)
    {
        if (f->d.qid.path == path)
        {
            return f;
        }
    }
    return nil;
}

/* [standalone-contract:findfid]
 * EN: findfid: original server session/fid dispatch and callback-backed namespace operations.
 * RU: findfid: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * fid (u32int fid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Fid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fid *findfid(Client *c, u32int fid)
{
    Fid *f;
    for (f = c->fids; f && f->fid != fid; f = f->next)
        ;
    return f;
}

/* [standalone-contract:deletefid]
 * EN: deletefid: original server session/fid dispatch and callback-backed namespace operations.
 * RU: deletefid: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * d (Fid *d): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void deletefid(Client *c, Fid *d)
{
    Fid **f;

    cancelreads(c, d);

    for (f = &c->fids; *f; f = &(*f)->next)
    {
        if (*f == d)
        {
            decreff(d);
            decopen(d);
            *f = d->next;
            if (c->current == d)
            {
                c->current = nil;
            }
            if (d->aux != nil && d->auxfree != nil)
            {
                d->auxfree(d->aux);
            }
            styxfree(d);
            return;
        }
    }
}

/* Clunk session handles while the per-client backend context is still alive. */


/* [standalone-contract:deletefids]
 * EN: deletefids: original server session/fid dispatch and callback-backed namespace operations.
 * RU: deletefids: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void deletefids(Client *c)
{
    Fid *f, *g;
    Client *previous;

    previous = c->server->curc;
    c->server->curc = c;
    f = c->fids;
    c->fids = nil;
    c->current = nil;
    for (; f != nil; f = g)
    {
        g = f->next;
        decreff(f);
        decopen(f);
        if (f->open && c->server->ops->close != nil)
        {
            c->server->ops->close(f->qid, f->mode);
        }
        if (f->aux != nil && f->auxfree != nil)
        {
            f->auxfree(f->aux);
        }
        styxfree(f);
    }
    c->server->curc = previous;
}

/* [standalone-contract:newfid]
 * EN: newfid: original server session/fid dispatch and callback-backed namespace operations.
 * RU: newfid: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * fid (u32int fid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * Return: Fid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Fid *newfid(Client *c, u32int fid, Qid qid)
{
    Fid *f;

    f = styxmalloc(sizeof(Fid));
    ASSERT(f, "newfid");
    f->client = c;
    f->fid = fid;
    f->open = 0;
    f->dri = 0;
    f->qid = qid;
    f->next = c->fids;
    c->fids = f;
    increff(f);
    return f;
}

/* [standalone-contract:eqqid]
 * EN: eqqid: original server session/fid dispatch and callback-backed namespace operations.
 * RU: eqqid: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * a (Qid a): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * b (Qid b): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int eqqid(Qid a, Qid b)
{
    return a.path == b.path && a.vers == b.vers;
}

/* [standalone-contract:fidclone]
 * EN: fidclone: original server session/fid dispatch and callback-backed namespace operations.
 * RU: fidclone: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * old (Fid *old): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * fid (u32int fid): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Fid *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fid *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fid *fidclone(Fid *old, u32int fid)
