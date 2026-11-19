/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/fid-context implementation and native state.
 * RU: реализация tests/fid-context и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Per-open native context ownership across clunk, reset and native shutdown.
 * Владение native-контекстом открытия при clunk, сбросе и завершении. */
#define styxsend capture
#include "../styxserver.c"
#undef styxsend

static Styxserver server;
static uchar reply[MSGMAX];
static int nreply, nextid, released;

/* Capture one real-codec reply without retaining caller storage.
 * Скопировать ответ настоящего кодека, не сохраняя чужой буфер. */


/* [standalone-contract:capture]
 * EN: capture: tests/fid-context implementation and native state.
 * RU: capture: реализация tests/fid-context и нативное состояние.
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
    memmove(reply, buf, n);
    nreply = n;
    return n;
}

/* Fail explicitly in every build profile.
 * Явный отказ в каждом профиле сборки. */


/* [standalone-contract:check]
 * EN: check: tests/fid-context implementation and native state.
 * RU: check: реализация tests/fid-context и нативное состояние.
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
        fprint(2, "fid context: %s\n", why);
        exit(1);
    }
}

/* Each opening allocates a separate owned context, even for the same Qid.
 * Каждое открытие создаёт отдельный контекст, даже для одинакового Qid. */


/* [standalone-contract:openfile]
 * EN: openfile: tests/fid-context implementation and native state.
 * RU: openfile: реализация tests/fid-context и нативное состояние.
 * q (Qid *q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *openfile(Qid *q, int mode)
{
    int *id;
    USED(q);
    USED(mode);
    id = malloc(sizeof(*id));
    check(id != nil, "allocate context");
    *id = ++nextid;
    check(styxsetaux(&server, id) == 0, "context adoption");
    check(styxsetaux(&server, id) < 0, "context replacement accepted");
    return nil;
}

/* Observe only the current fid's own native handle.
 * Получить native-handle только текущего fid. */


/* [standalone-contract:readfile]
 * EN: readfile: tests/fid-context implementation and native state.
 * RU: readfile: реализация tests/fid-context и нативное состояние.
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
    int *id = styxgetaux(&server);
    USED(q.path);
    USED(offset);
    check(id != nil, "missing current context");
    if (*n > 0)
    {
        buf[0] = *id;
        *n = 1;
    }
    return nil;
}

/* Consume the context exactly once, before its client backend is released.
 * Поглотить контекст ровно один раз, до освобождения backend клиента. */


/* [standalone-contract:freecontext]
 * EN: freecontext: tests/fid-context implementation and native state.
 * RU: freecontext: реализация tests/fid-context и нативное состояние.
 * aux (void *aux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void freecontext(void *aux)
{
    check(aux != nil, "empty context cleanup");
    released++;
    free(aux);
}

/* Execute one encoded request, then ensure the borrowed scope ended.
 * Выполнить кодированный запрос и проверить завершение заимствования. */


/* [standalone-contract:request]
 * EN: request: tests/fid-context implementation and native state.
 * RU: request: реализация tests/fid-context и нативное состояние.
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
    check(c->nread > 0, "encode");
    c->state = CNREAD;
    nreply = 0;
    server.curc = c;
    run(c);
    check(styxgetaux(&server) == nil, "context escaped dispatch");
    check(nreply > 0 && convM2S(reply, nreply, &out) == nreply, "decode");
    return out;
}

/* Two equal Qids must never alias context or double-close at reset/disconnect.
 * Два одинаковых Qid не разделяют контекст и не закрываются дважды. */


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
    Client c;
    Fcall f, out;
    int id;
    memset(&ops, 0, sizeof(ops));
    memset(&c, 0, sizeof(c));
    memset(table, 0, sizeof(table));
    server.ops = &ops;
    server.ftab = table;
    ops.open = openfile;
    ops.read = readfile;
    ops.freefid = freecontext;
    c.server = &server;
    newfid(&c, 1, styxqid(99, 0));
    newfid(&c, 2, styxqid(99, 0));
    memset(&f, 0, sizeof(f));
    f.type = Topen;
    f.mode = OREAD;
    f.fid = 1;
    out = request(&c, f);
    check(out.type == Ropen, "first open");
    f.fid = 2;
    out = request(&c, f);
    check(out.type == Ropen, "second open");
    f.type = Tread;
    f.count = 1;
    out = request(&c, f);
    id = (uchar)out.data[0];
    f.fid = 1;
    out = request(&c, f);
    check(out.type == Rread && (uchar)out.data[0] != id, "shared context between equal Qids");
    f.type = Tclunk;
    out = request(&c, f);
    check(out.type == Rclunk && released == 1, "clunk cleanup");
    f.type = Tversion;
    f.tag = NOTAG;
    f.msize = 1024;
    f.version = VERSION9P;
    out = request(&c, f);
    check(out.type == Rversion && released == 2, "reset cleanup");
    deletefids(&c);
    check(released == 2, "double cleanup");
    print("Styx per-fid native context: passed\n");
    return 0;
}
