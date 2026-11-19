/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/native64 implementation and native state.
 * RU: реализация tests/native64 и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Exercise original server dispatch with real wire codecs and a captured transport.
 * Проверка исходного сервера с настоящими кодеками и перехватом ответа. */
#define styxsend test_send
#include "../styxserver.c"
#undef styxsend

static uchar reply[MSGMAX];
static int nreply;
static ulong reported;
static int calls;
static ulong capacity;
static vlong seenoffset;

/* Capture the server reply; no socket or client buffer is retained.
 * Копирует ответ сервера; сокет и буфер клиента не сохраняются. */


/* [standalone-contract:test_send]
 * EN: test_send: tests/native64 implementation and native state.
 * RU: test_send: реализация tests/native64 и нативное состояние.
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
int test_send(Styxserver *s, int fd, char *buf, int n, int flags)
{
    USED(s);
    USED(fd);
    USED(flags);
    if (n < 0 || n > sizeof(reply))
    {
        exit(2);
    }
    memmove(reply, buf, n);
    nreply = n;
    return n;
}

/* Report the configured count and retain the full request offset.
 * Возвращает заданное число байтов и запоминает полное смещение. */


/* [standalone-contract:io]
 * EN: io: tests/native64 implementation and native state.
 * RU: io: реализация tests/native64 и нативное состояние.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
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
static char *io(Qid qid, char *buf, ulong *n, vlong offset)
{
    USED(qid.path);
    capacity = *n;
    if (*n > 0)
    {
        buf[0] = 'x';
    }
    *n = reported;
    seenoffset = offset;
    calls++;
    return nil;
}

/* Fail explicitly so checks remain active in release builds.
 * Явная ошибка сохраняет проверки в release-сборке. */


/* [standalone-contract:check]
 * EN: check: tests/native64 implementation and native state.
 * RU: check: реализация tests/native64 и нативное состояние.
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
        fprint(2, "native64: %s\n", why);
        exit(1);
    }
}

/* Dispatch one encoded request and decode its copied response.
 * Выполняет один закодированный запрос и разбирает копию ответа. */


/* [standalone-contract:request]
 * EN: request: tests/native64 implementation and native state.
 * RU: request: реализация tests/native64 и нативное состояние.
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
    check(c->nread > 0, "request encoding");
    c->state = CNREAD;
    nreply = 0;
    run(c);
    memset(&out, 0, sizeof(out));
    check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "reply decoding");
    check(out.tag == f.tag, "reply tag");
    return out;
}

/* Cover full-width IDs, same-fid walk lifetime and native count boundaries.
 * Проверяет полные ID, время жизни same-fid walk и границы native-счётчиков. */


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
    Styxserver s;
    Styxops ops;
    Client c;
    Styxfile *table[TABSZ];
    Fid *a, *b;
    Fcall f, out;
    Qid q;
    int before;

    memset(&s, 0, sizeof(s));
    memset(&ops, 0, sizeof(ops));
    memset(&c, 0, sizeof(c));
    memset(table, 0, sizeof(table));
    s.ops = &ops;
    s.ftab = table;
    c.server = &s;
    c.uname = "test";
    ops.read = io;
    ops.write = io;
    q = styxqid(0xfedcba9876543210ULL, 0);
    check(q.path == 0xfedcba9876543210ULL, "qid helper truncates path");
    a = newfid(&c, 0xf1234567U, q);
    b = newfid(&c, 0x81234567U, q);
    check(findfid(&c, 0xf1234567U) == a && findfid(&c, 0x81234567U) == b, "fid collision");
    memset(&f, 0, sizeof(f));
    f.type = Twalk;
    f.tag = 7;
    f.fid = a->fid;
    f.newfid = a->fid;
    out = request(&c, f);
    check(out.type == Rwalk && findfid(&c, f.fid) == a, "same-fid walk");
    f.newfid = b->fid;
    out = request(&c, f);
    check(out.type == Rerror && findfid(&c, b->fid) == b, "existing newfid replaced");
    f.newfid = 123;
    f.nwname = 1;
    f.wname[0] = "child";
    out = request(&c, f);
    check(out.type == Rerror && findfid(&c, 123) == nil, "walk through file killed server or leaked fid");
    a->open = 1;
    a->mode = ORDWR;
    f.type = Tread;
    f.count = 3;
    f.offset = 0x1234567887654321LL;
    reported = 1;
    out = request(&c, f);
    check(out.type == Rread && out.count == 1 && out.data[0] == 'x', "partial read");
    check(seenoffset == f.offset, "offset truncation");
    reported = 0;
    out = request(&c, f);
    check(out.type == Rread && out.count == 0, "zero read");
    f.count = 0xffffffffU;
    out = request(&c, f);
    check(out.type == Rread && capacity <= sizeof(c.data) - IOHDRSZ, "unbounded read buffer");
    f.count = 3;
    reported = (1UL << 32) + 1;
    out = request(&c, f);
    check(out.type == Rerror, "oversized native read count accepted");
    a->mode = OWRITE;
    before = calls;
    out = request(&c, f);
    check(out.type == Rerror && calls == before, "read of write-only fid");
    f.type = Twrite;
    f.data = "abc";
    a->mode = OREAD;
    out = request(&c, f);
    check(out.type == Rerror && calls == before, "write of read-only fid");
    a->mode = ORDWR;
    reported = 2;
    out = request(&c, f);
    check(out.type == Rwrite && out.count == 2, "partial write");
    reported = (1UL << 32) + 2;
    out = request(&c, f);
    check(out.type == Rerror, "oversized native write count accepted");
    deletefids(&c);
    print("native64 server: passed\n");
    return 0;
}
