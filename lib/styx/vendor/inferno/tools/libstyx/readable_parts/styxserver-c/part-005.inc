/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

/* [standalone-contract:styxsetaux]
 * EN: styxsetaux: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxsetaux: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * aux (void *aux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxsetaux(Styxserver *server, void *aux)
{
    Fid *f;
    if (server == nil || server->curc == nil || server->ops->freefid == nil || aux == nil)
    {
        return -1;
    }
    f = server->curc->current;
    if (f == nil || f->aux != nil)
    {
        return -1;
    }
    f->aux = aux;
    return 0;
}

/* Return a borrowed native context, never a VM or wire pointer. */


/* [standalone-contract:styxgetaux]
 * EN: styxgetaux: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxgetaux: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: void *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *styxgetaux(Styxserver *server)
{
    if (server == nil || server->curc == nil || server->curc->current == nil)
    {
        return nil;
    }
    return server->curc->current->aux;
}

/* [standalone-contract:styxinit]
 * EN: Initialize an original Styx server, its root and selected platform listener.
 * RU: Инициализировать исходный Styx-сервер, его корень и listener выбранной платформы.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * ops (Styxops *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * port (char *port): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * perm (int perm): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * needfile (int needfile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxinit(Styxserver *server, Styxops *ops, char *port, int perm, int needfile)
{
    int i;

    if (Debug)
    {
        fprint(2, "Initialising Styx server on port %s\n", port);
    }
    if (perm == -1)
    {
        perm = 0555;
    }
    server->ops = ops;
    server->connfd = -1;
    server->priv = nil;
    server->curc = nil;
    server->clients = nil;
    server->root = nil;
    server->ftab = (Styxfile **)malloc(TABSZ * sizeof(Styxfile *));
    if (server->ftab == nil)
    {
        return Enomem;
    }
    for (i = 0; i < TABSZ; i++)
    {
        server->ftab[i] = nil;
    }
    server->qidgen = Qroot + 1;
    if (styxinitsocket() < 0)
    {
        free(server->ftab);
        server->ftab = nil;
        return "styxinitsocket failed";
    }
    server->connfd = styxannounce(server, port);
    if (server->connfd < 0)
    {
        styxend(server);
        return "can't announce on network port";
    }
    styxinitwait(server);
    server->root = newfile(server, nil, 1, Qroot, "/", perm | DMDIR, eve);
    server->needfile = needfile;
    return nil;
}

/* Release common state after the owning platform admits synchronous teardown. */


/* [standalone-contract:styxendserver]
 * EN: styxendserver: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxendserver: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxendserver(Styxserver *server)
{
    Client *c;

    while ((c = server->clients) != nil)
    {
        server->curc = c;
        if (server->priv != nil)
        {
            styxfreeclient(server, c->fd);
        }
        freeclient(c);
    }
    server->curc = nil;
    if (server->root != nil)
    {
        sremove(server, server->root);
    }
    server->root = nil;
    free(server->ftab);
    server->ftab = nil;
    if (server->connfd >= 0)
    {
        styxclosesocket(server->connfd);
    }
    server->connfd = -1;
}

/* [standalone-contract:styxwait]
 * EN: Wait for the selected server backend to make connection/message work available.
 * RU: Ожидать появления работы соединений/сообщений у выбранного backend сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxwait(Styxserver *server)
{
    return styxwaitmsg(server);
}

/* [standalone-contract:styxprocess]
 * EN: Dispatch available original Styx messages through the selected server callbacks.
 * RU: Диспетчеризовать доступные исходные Styx-сообщения через callbacks сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxprocess(Styxserver *server)
{
    Client *c;
    int s;

    if (styxnewcall(server))
    {
        s = styxaccept(server);
        if (s >= 0)
        {
            newclient(server, s);
            styxnewclient(server, s);
        }
    }
    for (c = server->clients; c != nil;)
    {
        Client *next = c->next;

        server->curc = c;
        if (c->fd >= 0 && styxnewmsg(server, c->fd))
        {
            c->state |= CRECV;
        }
        if (c->state & (CNREAD | CRECV | CDISC))
        {
            if (!(c->state & CDISC))
            {
                do
                {
                    run(c);
                } while ((c->state & CNREAD) && !(c->state & CDISC));
            }
            if (c->state & CDISC)
            {
                styxfreeclient(server, c->fd);
                freeclient(c);
            }
        }
        c = next;
    }
    server->curc = nil;
    return nil;
}

/* [standalone-contract:styxclient]
 * EN: Borrow the current callback's original client context from its server.
 * RU: Заимствовать текущий original Client-контекст callback из его сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: Client *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Client *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Client *styxclient(Styxserver *server)
{
    return server->curc;
}

/* [standalone-contract:styxaddfile]
 * EN: styxaddfile: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxaddfile: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * pqid (Path pqid): Scalar operand of this operation; protocol/algorithm ranges are described below.
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
Styxfile *styxaddfile(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner)
{
    Styxfile *f, *parent;

    parent = styxfindfile(server, pqid);
    if (parent == nil || (parent->d.qid.type & QTDIR) == 0)
    {
        return nil;
    }
    f = newfile(server, parent, 0, qid, name, mode, owner);
    return f;
}

/* [standalone-contract:styxadddir]
 * EN: styxadddir: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxadddir: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * pqid (Path pqid): Scalar operand of this operation; protocol/algorithm ranges are described below.
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
Styxfile *styxadddir(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner)
{
    Styxfile *f, *parent;

    parent = styxfindfile(server, pqid);
    if (parent == nil || (parent->d.qid.type & QTDIR) == 0)
    {
        return nil;
    }
    f = newfile(server, parent, 1, qid, name, mode | DMDIR, owner);
    return f;
}

/* [standalone-contract:styxreadstr]
 * EN: styxreadstr: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxreadstr: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * off (ulong off): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (ulong n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: long; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
long styxreadstr(ulong off, char *buf, ulong n, char *str)
{
    int size;

    size = strlen(str);
    if (off >= size)
    {
        return 0;
    }
    if (off + n > size)
    {
        n = size - off;
    }
    memmove(buf, str + off, n);
    return n;
}

/* [standalone-contract:styxqid]
 * EN: Construct a scalar Qid from a stable backend identity and directory flag.
 * RU: Составить скалярный Qid из стабильного ID backend и признака каталога.
 * path (Path path): Selected object path or stable identity according to the declared type; no implicit authority is granted.
 * RU: Путь либо стабильный ID объекта по объявленному типу; не предоставляет неявных прав.
 * isdir (int isdir): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Qid; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Qid; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Qid styxqid(Path path, int isdir)
{
    Qid q;

    q.path = path;
    q.vers = 0;
    if (isdir)
    {
        q.type = QTDIR;
    }
    else
    {
        q.type = 0;
    }
    return q;
}

/* [standalone-contract:styxsetowner]
 * EN: styxsetowner: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxsetowner: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxsetowner(char *name)
{
    eve = name;
}
