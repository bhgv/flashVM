/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original server session/fid dispatch and callback-backed namespace operations.
 * RU: исходная диспетчеризация сессий/fid и callback-операции namespace.
 */
#include <lib9.h>
#include <styx.h>
#include "styxserver.h"
#include "styxaux.h"

#define MAXSTAT (MSGMAX - 9)
#define EMSGLEN 256 /* %r */

#define TABSZ 32    /* power of 2 */

static unsigned long boottime;
static char *eve = "inferno";
static int Debug = 0;

char Enomem[] = "out of memory";
char Eperm[] = "permission denied";
char Enodev[] = "no free devices";
char Ehungup[] = "write to hungup channel";
char Eexist[] = "file exists";
char Enonexist[] = "file does not exist";
char Ebadcmd[] = "bad command";
char Ebadarg[] = "bad arg in system call";
char Enofid[] = "no such fid";
char Enotdir[] = "not a directory";
char Eopen[] = "already open";
char Ebadfid[] = "bad fid";

/* client state */
enum
{
    CDISC = 01,
    CNREAD = 02,
    CRECV = 04,
};

typedef struct Walkqid Walkqid;

struct Fid
{
    Client *client;
    Fid *next;
    u32int fid;
    ushort open;
    ushort mode;  /* read/write */
    vlong offset; /* in file */
    int dri;      /* dirread index */
    Qid qid;
    void *aux;    /* owned by freefid callback until retirement */
};

struct Walkqid
{
    Fid *clone;
    int nqid;
    Qid qid[1];
};

#define ASSERT(A, B) styxassert((A) != 0, B)

static int hash(Path);
static void deletefids(Client *);
static void cancelattach(Client *);
static void cancelreads(Client *, Fid *);
static int readtag(Client *, ushort);
static void flushread(Client *, ushort);

/* [standalone-contract:styxfatal]
 * EN: styxfatal: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxfatal: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void styxfatal(char *fmt, ...)
{
    char buf[1024], *out;
    va_list arg;
    out = seprint(buf, buf + sizeof(buf), "Fatal error: ");
    va_start(arg, fmt);
    out = vseprint(out, buf + sizeof(buf), fmt, arg);
    va_end(arg);
    write(2, buf, out - buf);
    styxexit(1);
}

/* [standalone-contract:styxassert]
 * EN: styxassert: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxassert: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * vtrue (int vtrue): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * reason (char *reason): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void styxassert(int vtrue, char *reason)
{
    if (!vtrue)
    {
        styxfatal("assertion failed: %s\n", reason);
    }
}

/* [standalone-contract:styxmalloc]
 * EN: styxmalloc: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxmalloc: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * bytes (int bytes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: void *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *styxmalloc(int bytes)
{
    char *m = malloc(bytes);
    if (m == nil)
    {
        styxfatal(Enomem);
    }
    memset(m, 0, bytes);
    return m;
}

/* [standalone-contract:styxfree]
 * EN: styxfree: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxfree: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * p (void *p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxfree(void *p)
{
    free(p);
}

/* [standalone-contract:styxdebug]
 * EN: styxdebug: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxdebug: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxdebug()
{
    Debug = 1;
}

/* [standalone-contract:newclient]
 * EN: newclient: original server session/fid dispatch and callback-backed namespace operations.
 * RU: newclient: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Client *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Client *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Client *newclient(Styxserver *server, int fd)
{
    Client *c = (Client *)styxmalloc(sizeof(Client));

    if (Debug)
    {
        fprint(2, "New client at %lux\n", (ulong)c);
    }
    c->server = server;
    c->fd = fd;
    c->nread = 0;
    c->nc = 0;
    c->state = 0;
    c->msize = MSGMAX;
    c->versioned = -1;
    c->fids = nil;
    c->uname = strdup(eve);
    c->aname = strdup("");
    c->next = server->clients;
    server->clients = c;
    if (server->ops->newclient != nil && server->ops->newclient(c) != nil)
    {
        c->state |= CDISC;
    }
    return c;
}

/* [standalone-contract:freeclient]
 * EN: freeclient: original server session/fid dispatch and callback-backed namespace operations.
 * RU: freeclient: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void freeclient(Client *c)
{
    Client **p;
    Styxserver *server;

    if (Debug)
    {
        fprint(2, "Freeing client at %lux\n", (ulong)c);
    }
    server = c->server;
    cancelattach(c);
    cancelreads(c, nil);
    deletefids(c);
    if (server->ops->freeclient)
    {
        server->ops->freeclient(c);
    }
    for (p = &server->clients; *p; p = &(*p)->next)
    {
        if (*p == c)
        {
            styxclosesocket(c->fd);
            *p = c->next;
            free(c->uname);
            free(c->aname);
            if (server->curc == c)
            {
                server->curc = nil;
            }
            styxfree(c);
            return;
        }
    }
}

/* [standalone-contract:nbread]
 * EN: nbread: original server session/fid dispatch and callback-backed namespace operations.
 * RU: nbread: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * nr (int nr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int nbread(Client *c, int nr)
{
    int nb;

    if (c->state & CDISC)
    {
        return -1;
    }
    do
    {
        nb = styxrecv(c->server, c->fd, c->msg + c->nread, nr, 0);
    } while (nb < 0 && errno == EINTR);
    if (nb <= 0)
    {
        c->nread = 0;
        c->state |= CDISC;
        return -1;
    }
    c->nread += nb;
    return 0;
}

/* Keep incomplete TCP frames until their remaining bytes arrive. */


/* [standalone-contract:rd]
 * EN: rd: original server session/fid dispatch and callback-backed namespace operations.
 * RU: rd: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * r (Fcall *r): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1, 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int rd(Client *c, Fcall *r)
{
    u32int n;

    if (c->nc > 0)
    {
        if (c->nc > c->nread)
        {
            goto Bad;
        }
        c->nread -= c->nc;
        memmove(c->msg, c->msg + c->nc, c->nread);
        c->nc = 0;
    }
    if (c->state & CRECV)
    {
        c->state &= ~CRECV;
        if (nbread(c, MSGMAX - c->nread) != 0)
        {
            goto Bad;
        }
    }
    c->state &= ~CNREAD;
    if (c->nread < BIT32SZ)
    {
        return 0;
    }
    n = GBIT32((uchar *)c->msg);
    if (n < 7 || n > MSGMAX)
    {
        goto Bad;
    }
    if (c->nread >= 5 && c->msg[4] != Tversion && c->msize != 0 && n > c->msize)
    {
        goto Bad;
    }
    if (c->nread < n)
    {
        return 0;
    }
    c->nc = convM2S((uchar *)c->msg, n, r);
    if (c->nc != n)
    {
        goto Bad;
    }
    if (c->nread > n)
    {
        c->state |= CNREAD;
    }
    return 1;
Bad:
    c->state |= CDISC | CNREAD;
    r->ename = "invalid or closed Styx stream";
    return -1;
}

/* Complete one encoded reply; never discard a short transport write. */


/* [standalone-contract:wr]
 * EN: wr: original server session/fid dispatch and callback-backed namespace operations.
 * RU: wr: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * r (Fcall *r): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int wr(Client *c, Fcall *r)
{
    int n, sent, m;
    char buf[MSGMAX];

    n = convS2M(r, (uchar *)buf, sizeof(buf));
    if (n > (c->msize != 0 ? c->msize : MSGMAX))
    {
        r->type = Rerror;
        r->ename = "reply exceeds negotiated msize";
        n = convS2M(r, (uchar *)buf, sizeof(buf));
    }
    if (n <= 0)
    {
        goto Bad;
    }
    for (sent = 0; sent < n; sent += m)
    {
        m = styxsend(c->server, c->fd, buf + sent, n - sent, 0);
        if (m < 0 && errno == EINTR)
        {
            m = 0;
            continue;
        }
        if (m <= 0 || m > n - sent)
        {
            goto Bad;
        }
    }
    return sent;
Bad:
    c->state |= CDISC | CNREAD;
    return -1;
}

/* [standalone-contract:sremove]
 * EN: sremove: original server session/fid dispatch and callback-backed namespace operations.
 * RU: sremove: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * f (Styxfile *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void sremove(Styxserver *server, Styxfile *f)
{
    Styxfile *s, *next, **p;

    if (f == nil)
    {
        return;
    }
    if (Debug)
    {
        fprint(2, "Remove file %s Qid=%llx\n", f->d.name, f->d.qid.path);
    }
    if (f->d.qid.type & QTDIR)
    {
        for (s = f->child; s != nil; s = next)
