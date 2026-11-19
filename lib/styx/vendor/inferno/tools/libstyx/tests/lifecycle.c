/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: tests/lifecycle implementation and native state.
 * RU: реализация tests/lifecycle и нативное состояние.
 */
/* Copyright (c) 2026 bhgv
 * Real POSIX teardown and native fd ownership with a deterministic announce boundary.
 * Настоящее POSIX-завершение и владение fd с детерминированной границей announce. */
#define styxannounce announcefixture
#define styxinitsocket initfixture
#include "../styxserver.c"
#undef styxannounce
#undef styxinitsocket

static int listener, denied, initdenied, closed, released;
static Styxserver server;

/* Simulate socket initialization failure without changing platform globals.
 * Смоделировать отказ инициализации сокетов без изменения глобального состояния. */


/* [standalone-contract:initfixture]
 * EN: initfixture: tests/lifecycle implementation and native state.
 * RU: initfixture: реализация tests/lifecycle и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int initfixture(void)
{
    return initdenied ? -1 : 0;
}

/* A real owned native fd replaces only listener creation; no TCP claim is made.
 * Настоящий fd заменяет только создание listener; это не проверка TCP. */


/* [standalone-contract:announcefixture]
 * EN: announcefixture: tests/lifecycle implementation and native state.
 * RU: announcefixture: реализация tests/lifecycle и нативное состояние.
 * s (Styxserver *s): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * port (char *port): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int announcefixture(Styxserver *s, char *port)
{
    USED(s);
    USED(port);
    listener = denied ? -1 : dup(0);
    return listener;
}

/* Explicit checks remain active in release builds.
 * Явные проверки работают в release-сборке. */


/* [standalone-contract:check]
 * EN: check: tests/lifecycle implementation and native state.
 * RU: check: реализация tests/lifecycle и нативное состояние.
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
        fprint(2, "lifecycle: %s\n", why);
        exit(1);
    }
}

/* Clunk must precede backend release and see the proper current client.
 * Clunk предшествует освобождению backend и видит правильного клиента. */


/* [standalone-contract:closehandle]
 * EN: closehandle: tests/lifecycle implementation and native state.
 * RU: closehandle: реализация tests/lifecycle и нативное состояние.
 * q (Qid q): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *closehandle(Qid q, int mode)
{
    USED(q.path);
    USED(mode);
    check(styxclient(&server) != nil && styxclient(&server)->u != nil, "backend released before close");
    closed++;
    return nil;
}

/* The backend owns its private state until all fids have gone.
 * Backend владеет своим состоянием до освобождения всех fid. */


/* [standalone-contract:release]
 * EN: release: tests/lifecycle implementation and native state.
 * RU: release: реализация tests/lifecycle и нативное состояние.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *release(Client *c)
{
    check(c->fids == nil && c->u != nil, "invalid release order");
    c->u = nil;
    released++;
    return nil;
}

/* Cover nested roots, clients, native fds, repeat shutdown and failed initialization.
 * Проверить вложенное дерево, клиентов, fd, повторное закрытие и отказы init. */


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
    Client *client;
    Fid *fid;
    int a, b, owner;

    memset(&server, 0, sizeof(server));
    memset(&ops, 0, sizeof(ops));
    ops.close = closehandle;
    ops.freeclient = release;
    check(styxinit(&server, &ops, "0", 0555, 1) == nil, "init");
    owner = listener;
    check(owner >= 0 && server.priv != nil, "native owner");
    check(styxadddir(&server, Qroot, 1, "nested", 0555, "alice") != nil, "nested dir");
    check(styxaddfile(&server, 1, 2, "data", 0444, "alice") != nil, "nested file");
    a = dup(0);
    b = dup(0);
    check(a >= 0 && b >= 0, "client fds");
    client = newclient(&server, a);
    client->u = &server;
    styxnewclient(&server, a);
    fid = newfid(client, 7, styxqid(2, 0));
    fid->open = 1;
    client = newclient(&server, b);
    client->u = &server;
    styxnewclient(&server, b);
    fid = newfid(client, 8, styxqid(2, 0));
    fid->open = 1;
    check(styxend(&server) == nil, "shutdown");
    check(server.clients == nil && server.root == nil && server.ftab == nil && server.priv == nil &&
              server.curc == nil && server.connfd == -1,
          "retained server resources");
    check(closed == 2 && released == 2, "backend lifetime");
    check(fcntl(owner, F_GETFD) == -1 && errno == EBADF, "listener fd leaked");
    check(fcntl(a, F_GETFD) == -1 && errno == EBADF, "client fd leaked");
    check(fcntl(b, F_GETFD) == -1 && errno == EBADF, "second client fd leaked");
    check(styxend(&server) == nil && closed == 2 && released == 2, "double shutdown");
    denied = 1;
    check(styxinit(&server, &ops, "0", 0555, 1) != nil && server.ftab == nil && server.priv == nil,
          "announce failure leak");
    initdenied = 1;
    check(styxinit(&server, &ops, "0", 0555, 1) != nil && server.ftab == nil, "socket init failure leak");
    initdenied = denied = 0;
    check(styxinit(&server, &ops, "0", 0555, 1) == nil && styxend(&server) == nil, "reinitialization");
    print("Styx POSIX lifecycle: passed\n");
    return 0;
}
