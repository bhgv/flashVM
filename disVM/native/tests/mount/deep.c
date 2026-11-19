/* Copyright (c) 2026 bhgv
 * EN: Original C libstyx serves a synthetic deep hierarchy for multi-batch walks.
 * An acknowledged stop requests shutdown only after the sole client disconnects;
 * its opened fids must still accept reads and Tclunk replies until then.
 * RU: Исходная C libstyx выдаёт deep hierarchy для multi-batch walks. Stop только
 * запрашивает завершение после disconnect единственного клиента; его opened fid
 * до этого принимают read и подтверждённый Tclunk.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>

static Styxserver server;   /* Sole native owner / Единственный native owner. */
static int stop_requested;  /* ctl acknowledged; client still owns fids / Stop принят, fid ещё живы. */
static int stopping;        /* Sole client retired after its fids / Клиент завершён после fid. */
static int closed;          /* Successful-open retirements / Завершённые successful opens. */
static int clients;         /* Final backend retirements / Финальные backend retirements. */


/* EN: Resolve borrowed component name against mutable qid: seventeen d directories,
 * then file; ctl is a root sibling. Return nil after updating qid, Enonexist otherwise.
 * No resource is opened here. Only the original server owner calls this fixture.
 * RU: Разрешить name по qid: семнадцать каталогов d, затем file; ctl лежит в root.
 * Вернуть nil и изменить qid при успехе, иначе Enonexist. Ресурс здесь не открывается;
 * fixture вызывает только исходный server owner.
 */
static char *walk(Qid *qid, char *name)
{
    if (qid->path == 0 && !strcmp(name, "ctl"))
    {
        *qid = styxqid(100, 0);
        return nil;
    }
    if (qid->path < 17 && !strcmp(name, "d"))
    {
        *qid = styxqid(qid->path + 1, 1);
        return nil;
    }
    if (qid->path == 17 && !strcmp(name, "file"))
    {
        *qid = styxqid(18, 0);
        return nil;
    }
    return Enonexist;
}


/* EN: Borrow qid/mode; allow only file OREAD or ctl OWRITE. Return nil/Eperm;
 * successful opens are counted by their eventual close callbacks, not here.
 * RU: Заимствовать qid/mode; разрешить только file OREAD или ctl OWRITE.
 * Вернуть nil/Eperm; successful opens учитываются при окончательном close.
 */
static char *openfile(Qid *qid, int mode)
{
    return (qid->path == 18 && mode == OREAD) || (qid->path == 100 && mode == OWRITE) ? nil : Eperm;
}


/* EN: Fill borrowed out for logical qid with static strings and exact length/mode.
 * Return nil; the original dispatcher packs the borrowed strings before reuse.
 * RU: Заполнить out по qid статическими строками, length/mode. Вернуть nil;
 * исходный dispatcher упаковывает заимствованные строки до повторного обращения.
 */
static char *statfile(Qid qid, Dir *out)
{
    memset(out, 0, sizeof(*out));
    out->qid = qid;
    out->name = qid.path == 18 ? "file" : qid.path == 100 ? "ctl" : "d";
    out->uid = out->gid = out->muid = "fixture";
    out->length = qid.path == 18 ? 5 : 0;
    out->mode = qid.path < 18 ? DMDIR | 0555 : qid.path == 18 ? 0444 : 0222;
    return nil;
}


/* EN: Read retained qid18 into borrowed bytes of incoming *n capacity at offset.
 * Publish exact count including EOF via styxreadstr; return nil/Eperm.
 * A stop request does not invalidate this already opened object.
 * RU: Читать удержанный qid18 в bytes ёмкостью входного *n по offset. Задать
 * точный count, включая EOF, через styxreadstr; вернуть nil/Eperm. Stop request
 * не лишает доступа к уже открытому объекту.
 */
static char *readfile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    if (qid.path != 18 || offset < 0)
    {
        return Eperm;
    }
    *n = styxreadstr(offset, bytes, *n, "depth");
    return nil;
}


/* EN: Accept exactly four borrowed stop bytes on ctl at offset0; retain no input.
 * Set only stop_requested and return nil with unchanged *n acknowledgement.
 * The server must continue serving this client's final Tclunk messages. Final
 * shutdown is selected in freeclient, after original libstyx retires all fids.
 * RU: Принять ровно четыре stop bytes на ctl при offset0, не сохраняя input.
 * Выставить только stop_requested, вернуть nil с прежним *n acknowledgement.
 * Сервер продолжает обслуживать Tclunk клиента. Shutdown выбирается в freeclient
 * после освобождения всех fid исходной libstyx.
 */
static char *writefile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    if (qid.path != 100 || offset != 0 || *n != 4 || memcmp(bytes, "stop", 4))
    {
        return Eperm;
    }
    stop_requested = 1;
    return nil;
}


/* EN: Record one successful-open retirement; qid/mode are intentionally unused.
 * Return nil. Unopened partial-walk fids do not invoke this callback.
 * RU: Учесть один successful-open retirement; qid/mode намеренно не используются.
 * Вернуть nil. Unopened partial-walk fid не вызывают этот callback.
 */
static char *closefile(Qid qid, int mode)
{
    USED(qid.path);
    USED(mode);
    closed++;
    return nil;
}


/* EN: Original libstyx calls this after deleting the sole client's fids. client
 * is borrowed and unused. Count retirement and publish loop exit only when stop
 * was requested. Do not call styxend reentrantly; main does it after dispatch.
 * RU: Исходная libstyx вызывает callback после удаления fid клиента. client
 * заимствован и не используется. Учесть retirement, выбрать выход только после
 * stop request. Не вызывать styxend рекурсивно: main сделает это после dispatch.
 */
static char *freeclient(Client *client)
{
    USED(client);
    clients++;
    if (stop_requested)
    {
        stopping = 1;
    }
    return nil;
}


/* EN: Own one original POSIX Styx server and its serial callbacks. Print its
 * selected TCP port, serve until stop+client retirement, then require exactly two
 * opened-handle closes and one client release. Return 1/2 setup, 3 dispatch, 4
 * cleanup/count failure, otherwise 0. This is a single-client fixture, not a
 * production service shutdown protocol; external test drivers bound its lifetime.
 * RU: Владеть POSIX Styx server и serial callbacks. Вывести TCP port, обслуживать
 * до stop+client retirement; затем требовать ровно два close и один client release.
 * Коды: 1/2 setup, 3 dispatch, 4 cleanup/count error, иначе 0. Это single-client
 * fixture, не production shutdown protocol; lifetime ограничивает test driver.
 */
int main(void)
{
    Styxops ops;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    memset(&ops, 0, sizeof(ops));
    ops.walk = walk;
    ops.open = openfile;
    ops.stat = statfile;
    ops.read = readfile;
    ops.write = writefile;
    ops.close = closefile;
    ops.freeclient = freeclient;
    if (styxinit(&server, &ops, "0", 0555, 0) != nil)
    {
        return 1;
    }
    if (getsockname(server.connfd, (struct sockaddr *)&address, &length) < 0)
    {
        return 2;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        if (styxwait(&server) != nil || styxprocess(&server) != nil)
        {
            return 3;
        }
    }
    if (styxend(&server) != nil || closed != 2 || clients != 1)
    {
        return 4;
    }
    print("PASS deep server: closed 2 handles, released 1 client\n");
    return 0;
}
