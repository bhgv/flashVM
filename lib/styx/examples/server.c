/* Copyright (c) 2026 bhgv
 * EN: Original libstyx callback server over a replaceable, lazily queried catalog backend.
 * RU: Callback-сервер исходной libstyx над заменяемым лениво запрашиваемым catalog backend.
 * standalone-reviewed: authored teaching contracts; executable tokens unchanged.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "catalog.h"
static volatile sig_atomic_t stopping; /* Signal handler publishes shutdown only; callbacks own all backend work. */
static CatalogRow metadata; /* Original synchronous dispatcher packs stat strings before the next callback. */


/* EN: number is unused. Publish only the sig_atomic_t shutdown flag from SIGINT/SIGTERM; perform no
 * I/O, allocation or backend work in the handler.
 * RU: number не используется. Из SIGINT/SIGTERM выставить только sig_atomic_t shutdown flag; без
 * I/O, allocation и backend work в handler.
 */
static void stop_server(int number)
{
    (void)number;
    stopping = 1;
}


/* EN: Borrow row/out; initialize a standard Dir whose name points into row and ownership strings
 * are static. Preserve the full scalar Qid and rwx/directory bits; length is text bytes or zero for
 * a directory. Caller must pack before row storage changes. No allocation or I/O.
 * RU: row/out заимствованы; заполнить стандартный Dir со ссылкой name на row и static ownership
 * strings. Сохранить полный Qid и rwx/directory bits; length — text bytes либо 0 для каталога.
 * Упаковать до изменения row storage. Без allocation/I/O.
 */
static void attributes(const CatalogRow *row, Dir *out)
{
    memset(out, 0, sizeof(*out));
    out->qid = styxqid(row->id, (row->mode & CATALOG_DIR) != 0);
    out->mode = row->mode;
    out->name = (char *)row->name;
    out->uid = "demo";
    out->gid = "demo";
    out->muid = "";
    out->length = row->mode & CATALOG_DIR ? 0 : strlen(row->value);
}


/* EN: Borrow mutable qid and one NUL-terminated component name. Require current directory metadata,
 * resolve .. through its parent or scan immediate-child pages. Update qid only after success;
 * return nil or an error string. No complete tree/cache is created. Backend calls may block; only
 * the synchronous server owner calls this callback.
 * RU: Заимствовать qid и один NUL-terminated компонент name. Проверить directory metadata,
 * разрешить .. через parent либо просмотреть страницы детей. Менять qid только при успехе; вернуть
 * nil/error string. Без полного дерева/cache. Backend может блокировать; callback вызывает только
 * synchronous server owner.
 */
static char *walk_node(Qid *qid, char *name)
{
    CatalogRow rows[CATALOG_PAGE];
    size_t count;
    if (!catalog_query(0, qid->path, 0, rows, 1, &count) || count != 1)
    {
        return Enonexist;
    }
    if (!(rows[0].mode & CATALOG_DIR))
    {
        return "not a directory";
    }
    if (!strcmp(name, ".."))
    {
        if (!catalog_query(0, rows[0].parent, 0, rows, 1, &count) || count != 1)
        {
            return Enonexist;
        }
        *qid = styxqid(rows[0].id, (rows[0].mode & CATALOG_DIR) != 0);
        return nil;
    }
    for (size_t start = 0;; start += count)
    {
        if (!catalog_query(1, qid->path, start, rows, CATALOG_PAGE, &count))
        {
            return "backend query failed";
        }
        if (!count)
        {
            return Enonexist;
        }
        for (size_t index = 0; index < count; index++)
        {
            if (!strcmp(rows[index].name, name))
            {
                *qid = styxqid(rows[index].id, (rows[index].mode & CATALOG_DIR) != 0);
                return nil;
            }
        }
    }
}


/* EN: Query qid into static metadata and fill borrowed out. Return nil only for exactly one row,
 * otherwise Enonexist (including backend failure). String pointers survive until the next callback,
 * when the original dispatcher has already packed them. Serialized server-owner use only.
 * RU: Запросить qid в static metadata, заполнить out. Вернуть nil только для одной строки, иначе
 * Enonexist, включая backend error. Strings живут до следующего callback: исходный dispatcher уже
 * упаковал их. Только serialized server-owner use.
 */
static char *stat_node(Qid qid, Dir *out)
{
    size_t count;
    if (!catalog_query(0, qid.path, 0, &metadata, 1, &count) || count != 1)
    {
        return Enonexist;
    }
    attributes(&metadata, out);
    return nil;
}


/* EN: Borrow qid, inspect current metadata, and permit only exact OREAD/OWRITE when corresponding
 * rwx bits allow it; writes cannot target a directory. Successful open performs one catalog_pin;
 * failures acquire none. Return nil/error, never create/truncate or infer rights from a name.
 * Called by the server owner.
 * RU: Заимствовать qid, проверить metadata; разрешить только точный OREAD/OWRITE по rwx bits,
 * запись в каталог запрещена. Успех делает один catalog_pin, отказ — ни одного. Вернуть nil/error,
 * без create/truncate и вывода прав из имени. Вызывает server owner.
 */
static char *open_node(Qid *qid, int mode)
{
    Dir info;
    char *error = stat_node(*qid, &info);
    if (error)
    {
        return error;
    }
    if (mode == OREAD && (info.mode & 0444))
    {
        return catalog_pin(qid->path) ? nil : "cannot retain opened object";
    }
    return mode == OWRITE && !(info.mode & CATALOG_DIR) && (info.mode & 0222) && catalog_pin(qid->path) ? nil : Eperm;
}


/* EN: Release exactly one opened qid through catalog_unpin; mode is unused. Return nil. Operate on
 * the retained ID even after its name is hidden; never walk or reopen it.
 * RU: Освободить один opened qid через catalog_unpin; mode не используется. Вернуть nil. Работать с
 * удержанным ID даже после скрытия имени, без walk/reopen.
 */
static char *close_node(Qid qid, int mode)
{
    (void)mode;
    catalog_unpin(qid.path);
    return nil;
}


/* EN: Borrow qid, output bytes of incoming *size capacity and signed byte offset. Text reads return
 * confirmed current contents/EOF; directory reads encode whole Dir records from immediate-child
 * pages. On success publish actual *size and return nil; reject negative/missing objects, offsets
 * inside records or too-small first buffers. Errors may leave provisional output, which the
 * dispatcher must discard. This synchronous callback can block on backend I/O; no VM pause is
 * involved.
 * RU: Заимствовать qid, bytes ёмкостью входного *size и signed byte offset. Text read выдаёт
 * текущие contents/EOF; directory read — целые Dir из страниц детей. При успехе задать actual *size
 * и вернуть nil; отвергнуть negative/missing object, offset внутри записи или слишком малый первый
 * буфер. При error output provisional, dispatcher его отбрасывает. Callback синхронный, backend I/O
 * может блокировать; VM pause не нужен.
 */
static char *read_node(Qid qid, char *bytes, ulong *size, vlong offset)
{
    CatalogRow rows[CATALOG_PAGE];
    size_t count;
    if (offset < 0 || !catalog_query(0, qid.path, 0, rows, 1, &count) || count != 1)
    {
        return Enonexist;
    }
    if (!(rows[0].mode & CATALOG_DIR))
    {
        size_t length = strlen(rows[0].value);
        size_t available = (uvlong)offset < length ? length - (size_t)offset : 0;
        if (*size > available)
        {
            *size = available;
        }
        if (*size)
        {
            memcpy(bytes, rows[0].value + (size_t)offset, *size);
        }
        return nil;
    }
    uvlong position = 0;
    ulong written = 0;
    for (size_t start = 0;; start += count)
    {
        if (!catalog_query(1, qid.path, start, rows, CATALOG_PAGE, &count))
        {
            return "backend query failed";
        }
        if (!count)
        {
            *size = written;
            return position < (uvlong)offset ? "directory changed; rewind to zero" : nil;
        }
        for (size_t index = 0; index < count; index++)
        {
            Dir info;
            attributes(&rows[index], &info);
            uint required = sizeD2M(&info);
            if (position >= (uvlong)offset)
            {
                if (required > *size - written)
                {
                    *size = written;
                    return written ? nil : "directory buffer too small";
                }
                written += convD2M(&info, (uchar *)bytes + written, required);
            }
            else if (position + required > (uvlong)offset)
            {
                return "directory changed; rewind to zero";
            }
            position += required;
        }
    }
}


/* EN: Borrow qid/bytes for incoming *size bytes at offset. Delegate once, reject negative offset or
 * acknowledgement beyond the request; publish exact accepted *size only on success. Return nil or
 * Eperm. Do not retry uncertain work, reopen a name or treat a short write as a full one.
 * RU: Заимствовать qid/bytes длиной *size по offset. Делегировать один раз; отвергнуть negative
 * offset и count больше запроса. Публиковать точный accepted *size только при успехе, вернуть
 * nil/Eperm. Без replay, reopen имени и подмены short write полной записью.
 */
static char *write_node(Qid qid, char *bytes, ulong *size, vlong offset)
{
    long accepted = offset < 0 ? -1 : catalog_write(qid.path, bytes, *size, offset);
    if (accepted < 0 || (ulong)accepted > *size)
    {
        return Eperm;
    }
    *size = accepted;
    return nil;
}


/* EN: argc/argv select port (default 0) and optional backend source. Own one original libstyx
 * server and serialized callbacks; print actual loopback-accessible TCP port for clients.
 * SIGINT/SIGTERM requests loop exit, then styxend retires fids. Return 1 on setup, 2 on loop
 * failure, otherwise styxend status 0/3. Startup failure terminates this process; it is not an
 * embedding/restart API or asynchronous backend scheduler.
 * RU: argc/argv задают порт (default 0) и необязательный backend source. Владеть исходным libstyx
 * server и serialized callbacks; вывести фактический TCP port для клиентов. SIGINT/SIGTERM
 * запрашивает выход, затем styxend освобождает fid. Коды: 1 setup, 2 loop failure, иначе 0/3 по
 * styxend. Startup failure завершает процесс; это не embedding/restart API и не asynchronous
 * scheduler.
 */
int main(int argc, char **argv)
{
    Styxserver server;
    Styxops ops;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    struct sigaction action;
    memset(&ops, 0, sizeof(ops));
    memset(&action, 0, sizeof(action));
    action.sa_handler = stop_server;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    ops.walk = walk_node;
    ops.open = open_node;
    ops.stat = stat_node;
    ops.read = read_node;
    ops.write = write_node;
    ops.close = close_node;
    if (!catalog_init(argc > 2 ? argv[2] : NULL))
    {
        fprint(2, "backend initialization failed\n");
        return 1;
    }
    char *error = styxinit(&server, &ops, argc > 1 ? argv[1] : "0", 0555, 0);
    if (error || getsockname(server.connfd, (struct sockaddr *)&address, &length))
    {
        fprint(2, "server startup failed: %s\n", error ? error : "socket");
        return 1;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        if (styxwait(&server) || (!stopping && styxprocess(&server)))
        {
            if (!stopping)
            {
                styxend(&server);
                return 2;
            }
        }
    }
    return styxend(&server) ? 3 : 0;
}
