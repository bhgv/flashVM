/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.h; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original server session/fid dispatch and callback-backed namespace operations.
 * RU: исходная диспетчеризация сессий/fid и callback-операции namespace.
 */

#define Qroot 0

#define MSGMAX ((((8192 + 128) * 2) + 3) & ~3)

extern char Enomem[];    /* out of memory */
extern char Eperm[];     /* permission denied */
extern char Enodev[];    /* no free devices */
extern char Ehungup[];   /* i/o on hungup channel */
extern char Eexist[];    /* file exists */
extern char Enonexist[]; /* file does not exist */
extern char Ebadcmd[];   /* bad command */
extern char Ebadarg[];   /* bad arguments */

typedef uvlong Path;
typedef struct Styxserver Styxserver;
typedef struct Styxops Styxops;
typedef struct Styxfile Styxfile;
typedef struct Client Client;
typedef struct Fid Fid;
typedef struct StyxAttach StyxAttach;
typedef struct StyxRead StyxRead; /* Owned owner-thread deferred byte read. */

struct Styxserver
{
    Styxops *ops;
    Path qidgen;
    int connfd;
    int needfile;
    Client *clients;
    Client *curc;
    Styxfile *root;
    Styxfile **ftab;
    void *priv; /* private */
};

struct Client
{
    Styxserver *server;
    Client *next;
    int fd;
    char msg[MSGMAX];
    uint nread;         /* valid bytes in msg (including nc)*/
    int nc;             /* bytes consumed from front of msg by convM2S */
    char data[MSGMAX];  /* Tread/Rread data */
    int state;
    uint msize;         /* negotiated full-message bound */
    int versioned;      /* negative rejects unsupported sessions */
    Fid *fids;
    StyxAttach *attach; /* staged/pending attach, owned by dispatcher */
    StyxRead *reads;    /* Owned pending byte reads, at most128 per client. */
    StyxRead *reading;  /* Borrowed stack stage only during read callback. */
    Fid *current;       /* borrowed only during one dispatch */
    char *uname;        /* uid */
    char *aname;        /* attach name */
    void *u;
};

struct Styxops
{
    char *(*newclient)(Client *c);
    char *(*freeclient)(Client *c);

    char *(*attach)(char *uname, char *aname);
    char *(*walk)(Qid *qid, char *name);
    char *(*open)(Qid *qid, int mode);
    char *(*create)(Qid *qid, char *name, int perm, int mode);
    char *(*read)(Qid qid, char *buf, ulong *n, vlong offset);
    char *(*write)(Qid qid, char *buf, ulong *n, vlong offset);
    char *(*close)(Qid qid, int mode);
    char *(*remove)(Qid qid);
    char *(*stat)(Qid qid, Dir *d);
    char *(*wstat)(Qid qid, Dir *d);
    void (*freefid)(void *aux); /* consumes fid-owned native context, before freeclient */
};

struct Styxfile
{
    Dir d;
    Styxfile *parent;
    Styxfile *child;
    Styxfile *sibling;
    Styxfile *next;
    int ref;
    int open;
    void *u;
};

/* [standalone-contract:styxinit]
 * EN: Initialize an original Styx server, its root and selected platform listener.
 * RU: Инициализировать исходный Styx-сервер, его корень и listener выбранной платформы.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * ops (Styxops *ops): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * port (char *port): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * needfile (int needfile): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxinit(Styxserver *server, Styxops *ops, char *port, int perm, int needfile);


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
char *styxwait(Styxserver *server);


/* [standalone-contract:styxprocess]
 * EN: Dispatch available original Styx messages through the selected server callbacks.
 * RU: Диспетчеризовать доступные исходные Styx-сообщения через callbacks сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxprocess(Styxserver *server);


/* [standalone-contract:styxend]
 * EN: Retire the original server, its clients and opened fid state through their cleanup callbacks.
 * RU: Завершить исходный сервер, клиентов и открытые fid через callbacks освобождения.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxend(Styxserver *server);


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
Client *styxclient(Styxserver *server);


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
Styxfile *styxaddfile(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);


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
Styxfile *styxadddir(Styxserver *server, Path pqid, Path qid, char *name, int mode, char *owner);


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
int styxrmfile(Styxserver *server, Path qid);


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
Styxfile *styxfindfile(Styxserver *server, Path qid);


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
int styxperm(Styxfile *file, char *uid, int mode);


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
long styxreadstr(ulong off, char *buf, ulong n, char *str);


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
Qid styxqid(Path path, int isdir);


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
void *styxmalloc(int n);


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
void styxfree(void *p);


/* [standalone-contract:styxdebug]
 * EN: styxdebug: original server session/fid dispatch and callback-backed namespace operations.
 * RU: styxdebug: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxdebug(void);


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
void styxsetowner(char *);

/* Borrow current fid context; set transfers non-nil ownership only into an empty slot.
 * No automatic cloning: unopened walk clones start empty. Rebuild all consumers. */


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
void *styxgetaux(Styxserver *server);


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
int styxsetaux(Styxserver *server, void *aux);

/* Native owner-thread extension, no wire changes. Defer only inside attach callback.
 * Returned request is caller-owned; release with styxattachfree after retirement.
 * release(aux) runs once on success/error/flush/reset/disconnect, before freeclient.
 * Cleanup must not reenter dispatch/teardown or wait for external work.
 * These calls are serialized on the socket owner; worker threads signal a native
 * mailbox/fd instead of touching Client or request pointers. Rebuild consumers.
 * Native owner-thread API; defer только внутри attach callback, свой request.
 * release(aux) один раз при завершении/отмене; worker сообщает через mailbox/fd. */
/* EN/RU: Socket-owner root selection, pending NULL uses current attach;0/-1, no authority grant.
 * Выбор root на socket-owner; pending NULL — текущий attach;0/-1, без выдачи authority. */


/* [standalone-contract:styxattachroot]
 * EN: styxattachroot: libstyx/attach implementation and native state.
 * RU: styxattachroot: реализация libstyx/attach и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * pending (StyxAttach *pending): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * qid (Qid qid): Stable backend object identity/version/kind, not a native pointer or path string.
 * RU: Стабильные ID/версия/вид объекта backend, не native pointer и не строка пути.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxattachroot(Styxserver *server, StyxAttach *pending, Qid qid);


/* [standalone-contract:styxdeferattach]
 * EN: styxdeferattach: libstyx/attach implementation and native state.
 * RU: styxdeferattach: реализация libstyx/attach и нативное состояние.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * aux (void *aux): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxAttach *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: StyxAttach *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxAttach *styxdeferattach(Styxserver *, void *aux, void (*release)(void *));


/* [standalone-contract:styxattachclient]
 * EN: styxattachclient: libstyx/attach implementation and native state.
 * RU: styxattachclient: реализация libstyx/attach и нативное состояние.
 * request (StyxAttach *request): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Client *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Client *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Client *styxattachclient(StyxAttach *); /* Borrow live pending client, nil after retirement. */
