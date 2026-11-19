/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/stream implementation and native state.
 * RU: реализация tests/stream и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Original stream reader/writer with deterministic transport fragmentation.
 * Исходные чтение/запись с детерминированной фрагментацией транспорта. */
#define styxsend capture
#define styxrecv receive
#include "../styxserver.c"
#undef styxsend
#undef styxrecv

static uchar input[MSGMAX], output[MSGMAX];
static int available, consumed, produced, interruptsend;
static char longname[1201];
static int changed;

/* Supply only the bytes currently delivered by the native transport.
 * Выдать только уже поступившие байты транспорта. */


/* [standalone-contract:receive]
 * EN: receive: tests/stream implementation and native state.
 * RU: receive: реализация tests/stream и нативное состояние.
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
int receive(Styxserver *s, int fd, char *buf, int n, int flags)
{
    USED(s);
    USED(fd);
    USED(flags);
    if (n > available - consumed)
    {
        n = available - consumed;
    }
    memmove(buf, input + consumed, n);
    consumed += n;
    return n;
}

/* Short writes and EINTR must not lose bytes or duplicate the prefix.
 * Короткая запись и EINTR не должны терять байты или повторять префикс. */


/* [standalone-contract:capture]
 * EN: capture: tests/stream implementation and native state.
 * RU: capture: реализация tests/stream и нативное состояние.
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
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int capture(Styxserver *s, int fd, char *buf, int n, int flags)
{
    USED(s);
    USED(fd);
    USED(flags);
    if (interruptsend)
    {
        interruptsend = 0;
        errno = EINTR;
        return -1;
    }
    if (n > 3)
    {
        n = 3;
    }
    if (produced + n > sizeof(output))
    {
        exit(2);
    }
    memmove(output + produced, buf, n);
    produced += n;
    return n;
}

/* Explicit release-build check; failure terminates this isolated process.
 * Явная release-проверка; ошибка завершает изолированный процесс. */


/* [standalone-contract:check]
 * EN: check: tests/stream implementation and native state.
 * RU: check: реализация tests/stream и нативное состояние.
 * ok (int ok): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * text (char *text): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void check(int ok, char *text)
{
    if (!ok)
    {
        fprint(2, "stream: %s\n", text);
        exit(1);
    }
}

/* Return a long legal record without depending on a cached directory tree.
 * Вернуть длинную допустимую запись без кэшированного дерева каталогов. */


/* [standalone-contract:statrecord]
 * EN: statrecord: tests/stream implementation and native state.
 * RU: statrecord: реализация tests/stream и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *statrecord(Qid q, Dir *d)
{
    memset(d, 0, sizeof(*d));
    d->qid = q;
    d->name = longname;
    d->uid = d->gid = d->muid = "";
    return nil;
}

/* Confirm the decoded long name survived the bounded wstat string storage.
 * Проверить длинное имя после разбора в ограниченный буфер wstat. */


/* [standalone-contract:changerecord]
 * EN: changerecord: tests/stream implementation and native state.
 * RU: changerecord: реализация tests/stream и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * d (Dir *d): Native metadata/string references; encode fields rather than transmitting the structure layout.
 * RU: Нативные metadata/ссылки на строки; кодировать поля, не передавать layout структуры.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *changerecord(Qid q, Dir *d)
{
    USED(q.path);
    changed = d->name != nil && strlen(d->name) == 1200;
    return nil;
}

/* Send a request directly through the original dispatcher and captured writer.
 * Передать запрос исходному dispatcher с перехватом отправки ответа. */


/* [standalone-contract:request]
 * EN: request: tests/stream implementation and native state.
 * RU: request: реализация tests/stream и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * f (Fcall *f): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Fcall; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fcall; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fcall request(Client *c, Fcall *f)
{
    Fcall out;
    c->nc = 0;
    c->nread = convS2M(f, (uchar *)c->msg, sizeof(c->msg));
    check(c->nread > 0, "dispatch encode");
    c->state = CNREAD;
    produced = 0;
    run(c);
    check(convM2S(output, produced, &out) == produced, "dispatch decode");
    return out;
}

/* Exercise every split point, queued frames, malformed lengths and short replies.
 * Проверить все разрезы, очередь кадров, неверные размеры и короткую отправку. */


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
    Client c;
    Fcall f, result;
    int n, split;
    Styxserver server;
    Styxops ops;
    Styxfile *table[TABSZ];
    Dir d;

    memset(&f, 0, sizeof(f));
    f.type = Tread;
    f.tag = 9;
    f.fid = 0xf1234567;
    f.offset = 0x123456789abcdefLL;
    f.count = 20;
    n = convS2M(&f, input, sizeof(input));
    check(n > 0, "encode");
    for (split = 1; split < n; split++)
    {
        memset(&c, 0, sizeof(c));
        consumed = 0;
        available = split;
        c.state = CRECV;
        check(rd(&c, &result) == 0 && c.nread == split, "fragment discarded");
        available = n;
        c.state |= CRECV;
        check(rd(&c, &result) == 1 && result.fid == f.fid && result.offset == f.offset, "split decode");
    }
    memmove(input + n, input, n);
    memset(&c, 0, sizeof(c));
    consumed = 0;
    available = 2 * n;
    c.state = CRECV;
    check(rd(&c, &result) == 1 && (c.state & CNREAD), "queued frame flag");
    check(rd(&c, &result) == 1 && !(c.state & CNREAD), "queued frame decode");
    check(rd(&c, &result) == 0, "empty buffered stream");
    memset(&c, 0, sizeof(c));
    PBIT32(input, MSGMAX + 1);
    available = 4;
    consumed = 0;
    c.state = CRECV;
    check(rd(&c, &result) < 0 && (c.state & CDISC), "oversized frame");
    f.type = Rwrite;
    produced = 0;
    interruptsend = 1;
    memset(&c, 0, sizeof(c));
    n = wr(&c, &f);
    check(n == produced && produced > 0, "short write completion");
    check(convM2S(output, produced, &result) == produced && result.count == f.count, "short reply decode");
    memset(&server, 0, sizeof(server));
    memset(&ops, 0, sizeof(ops));
    memset(table, 0, sizeof(table));
    memset(&c, 0, sizeof(c));
    server.ftab = table;
    server.ops = &ops;
    c.server = &server;
    ops.stat = statrecord;
    ops.wstat = changerecord;
    memset(longname, 'n', 1200);
    newfid(&c, 42, styxqid(1, 0));
    memset(&f, 0, sizeof(f));
    f.type = Tstat;
    f.fid = 42;
    result = request(&c, &f);
    check(result.type == Rstat && result.nstat > 1200, "long stat truncated");
    statrecord(styxqid(1, 0), &d);
    f.type = Twstat;
    f.nstat = convD2M(&d, input, sizeof(input));
    f.stat = input;
    result = request(&c, &f);
    check(result.type == Rwstat && changed, "long wstat decode");
    deletefids(&c);
    print("Styx stream boundaries: passed\n");
    return 0;
}
