/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/session implementation and native state.
 * RU: реализация tests/session и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Original session dispatcher: negotiation, attach authority and fid lifetime.
 * Исходный dispatcher: согласование, полномочия attach и время жизни fid. */
#define styxsend capture
#define styxnewcall newcall
#define styxnewmsg newmsg
#define styxfreeclient forgetclient
#include "../styxserver.c"
#undef styxsend
#undef styxnewcall
#undef styxnewmsg
#undef styxfreeclient

static uchar reply[MSGMAX];
static int nreply, closed, freed, attached, capacity;
static Styxserver server;

/* No listener readiness is needed for a buffered malformed request.
 * Для ошибочного буферизованного запроса listener не нужен. */


/* [standalone-contract:newcall]
 * EN: newcall: tests/session implementation and native state.
 * RU: newcall: реализация tests/session и нативное состояние.
 * s (Styxserver *s): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int newcall(Styxserver *s)
{
    USED(s);
    return 0;
}

/* This test supplies complete buffered frames, without socket readiness.
 * Тест подаёт готовые кадры без событий сокета. */


/* [standalone-contract:newmsg]
 * EN: newmsg: tests/session implementation and native state.
 * RU: newmsg: реализация tests/session и нативное состояние.
 * s (Styxserver *s): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int newmsg(Styxserver *s, int fd)
{
    USED(s);
    USED(fd);
    return 0;
}

/* The deterministic test has no platform readiness registration to release.
 * В тесте нет платформенной регистрации readiness для освобождения. */


/* [standalone-contract:forgetclient]
 * EN: forgetclient: tests/session implementation and native state.
 * RU: forgetclient: реализация tests/session и нативное состояние.
 * s (Styxserver *s): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void forgetclient(Styxserver *s, int fd)
{
    USED(s);
    USED(fd);
}

/* Copy a whole native reply without retaining caller storage.
 * Копирует native-ответ, не сохраняя буфер вызывающей стороны. */


/* [standalone-contract:capture]
 * EN: capture: tests/session implementation and native state.
 * RU: capture: реализация tests/session и нативное состояние.
 * s (Styxserver *s): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * flags (int flags): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int capture(Styxserver *s, int fd, char *buf, int n, int flags)
{
    USED(s);
    USED(fd);
    USED(flags);
    if (n <= 0 || n > sizeof(reply))
    {
        exit(2);
    }
    memmove(reply, buf, n);
    nreply = n;
    return n;
}

/* Keep every check active in release mode.
 * Сохраняет все проверки в release-режиме. */


/* [standalone-contract:check]
 * EN: check: tests/session implementation and native state.
 * RU: check: реализация tests/session и нативное состояние.
 * ok (int ok): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * why (char *why): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void check(int ok, char *why)
{
    if (!ok)
    {
        fprint(2, "session: %s\n", why);
        exit(1);
    }
}

/* Reject a fixture identity; packet names are not authentication.
 * Отказ тестовому имени; имя в пакете не является аутентификацией. */


/* [standalone-contract:attach]
 * EN: attach: tests/session implementation and native state.
 * RU: attach: реализация tests/session и нативное состояние.
 * user (char *user): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *attach(char *user, char *name)
{
    USED(name);
    attached++;
    return strcmp(user, "denied") == 0 ? user : nil;
}

/* Fill exactly the admitted response capacity.
 * Заполняет ровно разрешённый объём ответа. */


/* [standalone-contract:readfile]
 * EN: readfile: tests/session implementation and native state.
 * RU: readfile: реализация tests/session и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (ulong *n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * offset (vlong offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *readfile(Qid q, char *buf, ulong *n, vlong offset)
{
    USED(q.path);
    USED(offset);
    capacity = *n;
    memset(buf, 'x', *n);
    return nil;
}

/* A legal record larger than negotiated msize must fail as a complete reply.
 * Допустимая запись больше msize должна вернуть целый ответ с ошибкой. */


/* [standalone-contract:statfile]
 * EN: statfile: tests/session implementation and native state.
 * RU: statfile: реализация tests/session и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *statfile(Qid q, Dir *d)
{
    static char name[301];
    memset(name, 'n', 300);
    memset(d, 0, sizeof(*d));
    d->qid = q;
    d->name = name;
    d->uid = d->gid = d->muid = "";
    return nil;
}

/* Close must run while the owning client backend is alive.
 * Close выполняется, пока backend клиента ещё существует. */


/* [standalone-contract:closefile]
 * EN: closefile: tests/session implementation and native state.
 * RU: closefile: реализация tests/session и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *closefile(Qid q, int mode)
{
    USED(q.path);
    USED(mode);
    check(styxclient(&server) != nil && styxclient(&server)->u != nil, "backend already released");
    closed++;
    return nil;
}

/* A rejected native client may own partial state that still needs release.
 * Отклонённый native-клиент может владеть частичным состоянием для освобождения. */


/* [standalone-contract:rejectclient]
 * EN: rejectclient: tests/session implementation and native state.
 * RU: rejectclient: реализация tests/session и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *rejectclient(Client *c)
{
    c->u = &server;
    return Eperm;
}

/* Release the backend only after all fids have been clunked.
 * Освободить backend только после закрытия всех fid. */


/* [standalone-contract:release]
 * EN: release: tests/session implementation and native state.
 * RU: release: реализация tests/session и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *release(Client *c)
{
    check(c->fids == nil, "client released before fids");
    c->u = nil;
    freed++;
    return nil;
}

/* Dispatch a real encoded request and decode its copied native reply.
 * Выполнить настоящий кодированный запрос и разобрать копию native-ответа. */


/* [standalone-contract:request]
 * EN: request: tests/session implementation and native state.
 * RU: request: реализация tests/session и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * f (Fcall f): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Fcall; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fcall; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fcall request(Client *c, Fcall f)
{
    Fcall out;
    c->nc = 0;
    c->nread = convS2M(&f, (uchar *)c->msg, sizeof(c->msg));
    check(c->nread > 0, "request encode");
    c->state = CNREAD;
    nreply = 0;
    server.curc = c;
    run(c);
    check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "reply decode");
    check(out.tag == f.tag, "tag changed");
    return out;
}

/* Negotiate independent connections and exercise failures without leaking handles.
 * Согласовать независимые соединения и проверить отказы без утечек дескрипторов. */


/* [standalone-contract:main]
 * EN: Validate the program's command-line contract, own its resources and return a process exit status.
 * RU: Проверить контракт командной строки, владеть ресурсами программы и вернуть код завершения.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int main(void)
{
    Styxops ops;
    Styxfile *table[TABSZ];
    Client *a, *b;
    Client bad;
    Fcall f, out;
    Fid *fid;
    int before;

    memset(&ops, 0, sizeof(ops));
    memset(table, 0, sizeof(table));
    server.ops = &ops;
    server.ftab = table;
    ops.attach = attach;
    ops.read = readfile;
    ops.stat = statfile;
    ops.close = closefile;
    ops.freeclient = release;
    a = newclient(&server, -1);
    b = newclient(&server, -1);
    a->u = b->u = &server;
    memset(&f, 0, sizeof(f));
    f.type = Tattach;
    f.tag = 1;
    f.fid = 3;
    f.afid = NOFID;
    f.uname = "alice";
    f.aname = "";
    out = request(a, f);
    check(out.type == Rerror && a->fids == nil && attached == 0, "attach before version");
    f.type = Tversion;
    f.tag = NOTAG;
    f.msize = 256;
    f.version = "9P2000.u";
    out = request(a, f);
    check(out.type == Rversion && out.msize == 256 && strcmp(out.version, VERSION9P) == 0, "negotiation");
    f.msize = 32;
    out = request(a, f);
    check(out.type == Rerror && a->msize == 256, "invalid msize accepted");
    f.msize = MSGMAX + 100;
    out = request(b, f);
    check(out.msize == MSGMAX && a->msize == 256, "independent limits");
    f.type = Tattach;
    f.tag = 1;
    f.fid = 7;
    f.afid = NOFID;
    f.uname = "denied";
    f.aname = "private";
    out = request(a, f);
    check(out.type == Rerror && a->fids == nil && strcmp(a->uname, eve) == 0 && a->aname[0] == 0,
          "failed attach leaked state");
    f.uname = "alice";
    out = request(a, f);
    check(out.type == Rattach && findfid(a, 7) != nil, "attach");
    f.fid = 8;
    f.uname = "bob";
    before = attached;
    out = request(a, f);
    check(out.type == Rerror && attached == before && strcmp(a->uname, "alice") == 0, "existing authority replaced");
    f.type = Tauth;
    out = request(a, f);
    check(out.type == Rerror, "fabricated authentication qid");
    fid = newfid(a, 42, styxqid(42, 0));
    fid->open = 1;
    fid->mode = OREAD;
    f.type = Tread;
    f.fid = 42;
    f.count = 4096;
    f.offset = 0;
    out = request(a, f);
    check(out.type == Rread && nreply <= 256 && capacity == 256 - IOHDRSZ, "read exceeds msize");
    f.type = Tstat;
    out = request(a, f);
    check(out.type == Rerror && nreply <= 256, "stat exceeds msize");
    f.type = Tversion;
    f.msize = 256;
    f.version = VERSION9P;
    out = request(a, f);
    check(out.type == Rerror && findfid(a, 42) == fid, "bad tag reset session");
    f.tag = NOTAG;
    f.version = "9P2099";
    out = request(a, f);
    check(out.type == Rversion && strcmp(out.version, "unknown") == 0 && a->fids == nil && closed == 1,
          "unknown session reset");
    f.type = Tattach;
    f.fid = 9;
    f.uname = "alice";
    out = request(a, f);
    check(out.type == Rerror && a->fids == nil, "unsupported version accepted attach");
    f.type = Tversion;
    f.version = VERSION9P;
    out = request(a, f);
    check(out.type == Rversion && a->versioned == 1, "renegotiation");
    memset(&bad, 0, sizeof(bad));
    bad.msize = 256;
    bad.nread = 5;
    PBIT32((uchar *)bad.msg, 257);
    bad.msg[4] = Twrite;
    check(rd(&bad, &out) < 0 && (bad.state & CDISC), "oversized incoming frame");
    fid = newfid(b, 99, styxqid(99, 0));
    fid->open = 1;
    freeclient(a);
    b->nread = 5;
    b->nc = 0;
    PBIT32((uchar *)b->msg, MSGMAX + 1);
    b->msg[4] = Twrite;
    b->state = CNREAD;
    styxprocess(&server);
    check(server.curc == nil, "dangling current client");
    check(closed == 2 && freed == 2 && server.clients == nil, "disconnect lifetime");
    ops.newclient = rejectclient;
    a = newclient(&server, -1);
    check(a->state & CDISC, "backend rejection ignored");
    styxprocess(&server);
    check(server.clients == nil && freed == 3 && closed == 2, "rejected client leaked");
    print("Styx session boundaries: passed\n");
    return 0;
}
