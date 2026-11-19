/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/directories implementation and native state.
 * RU: реализация tests/directories и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Live callback directories and whole-record cached pagination, without tree snapshots.
 * Живые callback-каталоги и целые записи при постраничном чтении, без снимков дерева. */
#define styxsend capture
#include "../styxserver.c"
#undef styxsend

static uchar reply[MSGMAX];
static int nreply, phase, reads, malformed;
static vlong seenoffset;

/* Copy one complete original-codec reply.
 * Скопировать один полный ответ исходного кодека. */


/* [standalone-contract:capture]
 * EN: capture: tests/directories implementation and native state.
 * RU: capture: реализация tests/directories и нативное состояние.
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

/* Keep release checks explicit and independent of assert/NDEBUG.
 * Проверки release не зависят от assert/NDEBUG. */


/* [standalone-contract:check]
 * EN: check: tests/directories implementation and native state.
 * RU: check: реализация tests/directories и нативное состояние.
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
        fprint(2, "directories: %s\n", why);
        exit(1);
    }
}

/* Generate the current directory on demand, using the real wire byte offset.
 * Выдать текущий каталог по запросу с настоящим байтовым смещением протокола. */


/* [standalone-contract:readlive]
 * EN: readlive: tests/directories implementation and native state.
 * RU: readlive: реализация tests/directories и нативное состояние.
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
static char *readlive(Qid q, char *buf, ulong *n, vlong offset)
{
    Dir d;
    uint size;
    USED(q.path);
    reads++;
    seenoffset = offset;
    if (malformed)
    {
        if (*n > 0)
        {
            buf[0] = 0;
            *n = 1;
        }
        return nil;
    }
    memset(&d, 0, sizeof(d));
    d.qid = styxqid(0xf123456789abcdefULL, 0);
    d.mode = phase ? 0640 : 0440;
    d.name = phase ? "later" : "first";
    d.uid = "alice";
    d.gid = "group";
    d.muid = "";
    size = sizeD2M(&d);
    if (offset != 0)
    {
        *n = 0;
        return nil;
    }
    if (*n < size)
    {
        return "buffer too small";
    }
    *n = convD2M(&d, (uchar *)buf, *n);
    return nil;
}

/* Provider-owned views must not fall back to a hidden cached child after denial.
 * Отказ провайдера не должен открывать скрытого ребёнка из внутреннего кэша. */


/* [standalone-contract:walklive]
 * EN: walklive: tests/directories implementation and native state.
 * RU: walklive: реализация tests/directories и нативное состояние.
 * q (Qid *q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *walklive(Qid *q, char *name)
{
    USED(q);
    USED(name);
    return Eperm;
}

/* Run an arbitrary encoded operation through the original dispatcher.
 * Выполнить кодированную операцию через исходный dispatcher. */


/* [standalone-contract:request]
 * EN: request: tests/directories implementation and native state.
 * RU: request: реализация tests/directories и нативное состояние.
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
    check(c->nread > 0, "encode");
    c->state = CNREAD;
    nreply = 0;
    run(c);
    check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "decode");
    return out;
}

/* Dispatch a real Tread for the retained directory fid.
 * Выполнить настоящий Tread для сохранённого fid каталога. */


/* [standalone-contract:readrequest]
 * EN: readrequest: tests/directories implementation and native state.
 * RU: readrequest: реализация tests/directories и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * count (ulong count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * offset (vlong offset): Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning.
 * RU: Позиция выбранного объекта; байты файла/кодированных записей каталога имеют протокольный смысл.
 * Return: Fcall; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fcall; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fcall readrequest(Client *c, ulong count, vlong offset)
{
    Fcall f;
    memset(&f, 0, sizeof(f));
    f.type = Tread;
    f.tag = 9;
    f.fid = 7;
    f.count = count;
    f.offset = offset;
    return request(c, &f);
}

/* Inspect the copied stat attributes, not just the returned byte count.
 * Проверить атрибуты stat, а не только количество байтов. */


/* [standalone-contract:record]
 * EN: record: tests/directories implementation and native state.
 * RU: record: реализация tests/directories и нативное состояние.
 * out (Fcall out): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * name (char *name): Component or schema name in this operation; the function validates its own encoding/length limits.
 * RU: Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void record(Fcall out, char *name, int mode)
{
    Dir d;
    char strings[MSGMAX];
    check(out.type == Rread && out.count > 0, "missing record");
    check(convM2D((uchar *)out.data, out.count, &d, strings) == out.count, "partial record");
    check(strcmp(d.name, name) == 0 && d.mode == mode, "stale directory attributes");
}

/* The same root fid must observe changes; cached reads never split or skip entries.
 * Тот же root fid видит изменения; кэш не дробит и не пропускает записи. */


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
    Fid *fid;
    Styxfile *table[TABSZ], *first;
    Fcall out, f;
    uint size;

    memset(&s, 0, sizeof(s));
    memset(&ops, 0, sizeof(ops));
    memset(&c, 0, sizeof(c));
    memset(table, 0, sizeof(table));
    s.ops = &ops;
    s.ftab = table;
    s.root = newfile(&s, nil, 1, Qroot, "/", 0555, "alice");
    c.server = &s;
    c.uname = "alice";
    fid = newfid(&c, 7, styxqid(Qroot, 1));
    fid->open = 1;
    fid->mode = OREAD;
    ops.read = readlive;
    out = readrequest(&c, 4096, 0);
    record(out, "first", 0440);
    phase = 1;
    out = readrequest(&c, 4096, 0);
    record(out, "later", 0640);
    check(reads == 2, "root cache bypassed live provider");
    out = readrequest(&c, 4096, 0x123456789abcdefLL);
    check(out.type == Rread && out.count == 0 && seenoffset == 0x123456789abcdefLL, "callback offset truncated");
    malformed = 1;
    out = readrequest(&c, 4096, 0);
    check(out.type == Rerror, "partial provider stat accepted");
    check(styxaddfile(&s, Qroot, 3, "hidden", 0444, "alice") != nil, "hidden cached child");
    ops.walk = walklive;
    newfid(&c, 9, styxqid(Qroot, 1));
    memset(&f, 0, sizeof(f));
    f.type = Twalk;
    f.tag = 10;
    f.fid = 9;
    f.newfid = 10;
    f.nwname = 1;
    f.wname[0] = "hidden";
    out = request(&c, &f);
    check(out.type == Rerror && findfid(&c, 10) == nil, "denied walk escaped into cached tree");
    styxrmfile(&s, 3);
    s.needfile = 1;
    first = styxaddfile(&s, Qroot, 1, "one", 0444, "alice");
    check(first != nil, "add cached child");
    size = sizeD2M(&first->d);
    out = readrequest(&c, size - 1, 0);
    check(out.type == Rerror && fid->dri == 0 && fid->offset == 0, "small buffer skipped entry or gave EOF");
    out = readrequest(&c, size, 0);
    record(out, "one", 0444);
    out = readrequest(&c, size, size + 1);
    check(out.type == Rerror, "invalid cached byte offset accepted");
    out = readrequest(&c, size, size);
    check(out.type == Rread && out.count == 0, "cached EOF");
    check(styxaddfile(&s, Qroot, 2, "two", 0644, "alice") != nil, "add live cached child");
    out = readrequest(&c, size, 0);
    record(out, "two", 0644);
    out = readrequest(&c, size, size);
    record(out, "one", 0444);
    deletefids(&c);
    styxrmfile(&s, 1);
    styxrmfile(&s, 2);
    /* Root teardown is accepted separately by the platform lifecycle test. */
    free(s.root->d.name);
    free(s.root->d.uid);
    free(s.root->d.gid);
    free(s.root);
    print("Styx live directories: passed\n");
    return 0;
}
