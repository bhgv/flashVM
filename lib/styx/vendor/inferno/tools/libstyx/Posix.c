/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: POSIX listener, readiness and socket byte transport for the original server.
 * RU: POSIX listener, readiness и socket byte transport исходного сервера.
 */
#define __EXTENSIONS__
#define _BSD_COMPAT
#include <lib9.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "styxserver.h"
#include "styxaux.h"

typedef struct Fdset Fdset;

struct Fdset
{
    fd_set infds, outfds, excfds, r_infds, r_outfds, r_excfds;
    int notifyfd; /* Borrowed native mailbox readiness fd, never closed here. */
};

/* [standalone-contract:styxinitsocket]
 * EN: styxinitsocket: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxinitsocket: POSIX listener, readiness и socket byte transport исходного сервера.
 * Parameters: none. Параметры: нет.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxinitsocket(void)
{
    return 0;
}

/* [standalone-contract:styxendsocket]
 * EN: styxendsocket: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxendsocket: POSIX listener, readiness и socket byte transport исходного сервера.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxendsocket(void)
{
}

/* [standalone-contract:styxclosesocket]
 * EN: styxclosesocket: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxclosesocket: POSIX listener, readiness и socket byte transport исходного сервера.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxclosesocket(int fd)
{
    close(fd);
}

/* [standalone-contract:styxannounce]
 * EN: styxannounce: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxannounce: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * port (char *port): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxannounce(Styxserver *server, char *port)
{
    struct sockaddr_in sin;
    int s, one;

    USED(server);
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
    {
        return s;
    }
    if (s >= FD_SETSIZE)
    {
        close(s);
        return -1;
    }
    one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) < 0)
    {
        fprint(2, "setsockopt failed\n");
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(atoi(port));
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
        close(s);
        return -1;
    }
    if (listen(s, 20) < 0)
    {
        close(s);
        return -1;
    }
    return s;
}

/* [standalone-contract:styxaccept]
 * EN: styxaccept: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxaccept: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxaccept(Styxserver *server)
{
    struct sockaddr_in sin;
    int s;
    socklen_t len;

    len = sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    s = accept(server->connfd, (struct sockaddr *)&sin, &len);
    if (s >= FD_SETSIZE)
    {
        close(s);
        return -1;
    }
    if (s < 0)
    {
        if (errno != EINTR)
        {
            fprint(2, "error in accept: %s\n", strerror(errno));
        }
    }
    return s;
}

/* [standalone-contract:styxinitwait]
 * EN: styxinitwait: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxinitwait: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxinitwait(Styxserver *server)
{
    Fdset *fs;

    server->priv = fs = malloc(sizeof(Fdset));
    FD_ZERO(&fs->infds);
    FD_ZERO(&fs->outfds);
    FD_ZERO(&fs->excfds);
    FD_SET(server->connfd, &fs->infds);
    fs->notifyfd = -1;
}

/* [standalone-contract:styxnewcall]
 * EN: styxnewcall: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxnewcall: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnewcall(Styxserver *server)
{
    Fdset *fs;

    fs = server->priv;
    return FD_ISSET(server->connfd, &fs->r_infds);
}

/* [standalone-contract:styxnewclient]
 * EN: styxnewclient: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxnewclient: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * s (int s): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxnewclient(Styxserver *server, int s)
{
    Fdset *fs;

    fs = server->priv;
    FD_SET(s, &fs->infds);
}

/* [standalone-contract:styxfreeclient]
 * EN: styxfreeclient: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxfreeclient: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * s (int s): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxfreeclient(Styxserver *server, int s)
{
    Fdset *fs;

    fs = server->priv;
    FD_CLR(s, &fs->infds);
}

/* [standalone-contract:styxnewmsg]
 * EN: styxnewmsg: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxnewmsg: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * s (int s): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnewmsg(Styxserver *server, int s)
{
    Fdset *fs;

    fs = server->priv;
    return FD_ISSET(s, &fs->r_infds) || FD_ISSET(s, &fs->r_excfds);
}

/* [standalone-contract:styxwaitmsg]
 * EN: styxwaitmsg: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxwaitmsg: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *styxwaitmsg(Styxserver *server)
{
    struct timeval seltime;
    int nfds;
    Fdset *fs;

    fs = server->priv;
    fs->r_infds = fs->infds;
    fs->r_outfds = fs->outfds;
    fs->r_excfds = fs->excfds;
    seltime.tv_sec = 10;
    seltime.tv_usec = 0L;
    nfds = select(sizeof(fd_set) * 8, &fs->r_infds, &fs->r_outfds, &fs->r_excfds, &seltime);
    if (nfds < 0)
    {
        FD_ZERO(&fs->r_infds);
        FD_ZERO(&fs->r_outfds);
        FD_ZERO(&fs->r_excfds);
        if (errno != EINTR)
        {
            return "error in select";
        }
    }
    return nil;
}

/* [standalone-contract:styxrecv]
 * EN: styxrecv: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxrecv: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * m (int m): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxrecv(Styxserver *server, int fd, char *buf, int n, int m)
{
    return recv(fd, buf, n, m);
}

/* [standalone-contract:styxsend]
 * EN: styxsend: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxsend: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * m (int m): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxsend(Styxserver *server, int fd, char *buf, int n, int m)
{
#ifdef MSG_NOSIGNAL
    m |= MSG_NOSIGNAL;
#endif
    return send(fd, buf, n, m);
}

/* [standalone-contract:styxexit]
 * EN: styxexit: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxexit: POSIX listener, readiness и socket byte transport исходного сервера.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void styxexit(int n)
{
    exit(n);
}

/* End the native socket owner once, after clunking clients and their backend state. */


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
char *styxend(Styxserver *server)
{
    if (server->ftab == nil)
    {
        return nil;
    }
    styxendserver(server);
    free(server->priv);
    server->priv = nil;
    styxendsocket();
    return nil;
}

/* EN/RU: Register borrowed native readiness input on the socket owner;0/-1.
 * Зарегистрировать borrowed native readiness fd на socket-owner;0/-1. */


/* [standalone-contract:styxnotifyfd]
 * EN: styxnotifyfd: POSIX listener, readiness and socket byte transport for the original server.
 * RU: styxnotifyfd: POSIX listener, readiness и socket byte transport исходного сервера.
 * server (Styxserver *server): Original server instance and dispatcher-owned state.
 * RU: Экземпляр исходного сервера и состояние его dispatcher.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxnotifyfd(Styxserver *server, int fd)
{
    Fdset *fs;
    if (server == nil || server->priv == nil || fd < -1 || fd >= FD_SETSIZE)
    {
        return -1;
    }
    fs = server->priv;
    if (fd >= 0 && fd != fs->notifyfd && FD_ISSET(fd, &fs->infds))
    {
        return -1;
    }
    if (fs->notifyfd >= 0)
    {
        FD_CLR(fs->notifyfd, &fs->infds);
    }
    fs->notifyfd = fd;
    if (fd >= 0)
    {
        FD_SET(fd, &fs->infds);
    }
    return 0;
}
