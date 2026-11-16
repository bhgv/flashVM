/* Copyright (c) 2026 bhgv
 * EN: Original C Styx mutable pathname fixture; held fids keep old directory/file Qids.
 * RU: Original C Styx fixture изменяемых pathname; held fids сохраняют прежние directory/file Qids.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
static Styxserver server; /* Original socket owner, no guest/runtime objects. */
static int replaced; /* Root source spelling now resolves to directory20 instead of10. */
static int swapped; /* Directory10/sub now resolves to directory21 instead of11. */
static int added; /* Live child added beneath held directory10. */
static int hidden; /* Directory10/item removed from walk/list, held file12 survives. */
static int denied; /* Actual file12 open/read denied dynamically. */
static int stopping; /* Fixture command terminates owner after client tests. */


/* EN/RU: Use64-bit node IDs so pinning never depends on a truncated path identity.
 * Использовать64-bit node IDs, чтобы pinning не зависел от усечённой identity. */
static Qid node(int id, int directory)
{
    return styxqid(0x100000000ULL+(uvlong)id, directory);
}


/* EN/RU: Decode known fixture Qids; directory Qroot keeps its standard cached identity.
 * Декодировать известные fixture Qids; directory Qroot сохраняет standard cached identity. */
static int identity(Qid qid)
{
    return qid.path == Qroot ? 0 : (int)(qid.path-0x100000000ULL);
}


/* EN/RU: Mutating lookup names never changes an existing fid's Qid.
 * Изменение lookup names не меняет Qid уже существующего fid. */
static char *walk(Qid *qid, char *name)
{
    int id = identity(*qid);
    if (!strcmp(name, ".."))
    {
        return Eperm;
    }
    if (id == 0)
    {
        if (!strcmp(name, "source"))
        {
            *qid = node(replaced ? 20 : 10, 1);
        }
        else if (!strcmp(name, "ctl"))
        {
            *qid = node(90, 0);
        }
        else
        {
            return Enonexist;
        }
        return nil;
    }
    if (id == 10 || id == 20)
    {
        if (!strcmp(name, "item") && !(id == 10 && hidden))
        {
            *qid = node(id+2, 0);
        }
        else if (!strcmp(name, "sub"))
        {
            *qid = node(id == 10 && swapped ? 21 : id+1, 1);
        }
        else if (!strcmp(name, "private"))
        {
            *qid = node(id+5, 0);
        }
        else if (id == 10 && added && !strcmp(name, "added"))
        {
            *qid = node(14, 0);
        }
        else
        {
            return Enonexist;
        }
        return nil;
    }
    if ((id == 11 || id == 21) && !strcmp(name, "item"))
    {
        *qid = node(id+2, 0);
        return nil;
    }
    return Enonexist;
}


/* EN/RU: Test access changes are authoritative at the operation, not inferred from stat mode.
 * Access changes authoritative в операции, не выводятся из stat mode. */
static char *openfile(Qid *qid, int mode)
{
    int id = identity(*qid);
    return id == 90 ? mode == OWRITE ? nil : Eperm :
        mode != OREAD || (id == 12 && denied) ? Eperm : nil;
}


/* EN/RU: Metadata for held objects stays usable after their original pathname is replaced/removed.
 * Metadata held objects остаётся доступной после замены/удаления исходного pathname. */
static char *statfile(Qid qid, Dir *out)
{
    int id = identity(qid);
    memset(out, 0, sizeof(*out));
    out->qid = qid;
    out->mode = (qid.type & QTDIR) ? DMDIR|0555 : id == 90 ? 0222 : 0444;
    out->length = (qid.type & QTDIR) ? 0 : 1;
    out->name = id == 0 ? "/" : id == 10 || id == 20 ? "source" :
        id == 11 || id == 21 ? "sub" : id == 90 ? "ctl" : id == 14 ? "added" :
        id == 15 || id == 25 ? "private" : "item";
    out->uid = out->gid = out->muid = "fixture";
    return nil;
}


/* EN/RU: Emit ordinary stat records for current directory members; never serialize/freeze a tree.
 * Выдать ordinary stat records текущего membership; дерево не сериализуется/не замораживается. */
static char *directory(Qid qid, char *bytes, ulong *count, vlong offset)
{
    const char *names[] = {"source", "ctl", "item", "sub", "private", "added"};
    int id = identity(qid);
    ulong used = 0;
    vlong position = 0;
    for (unsigned i = 0; i < nelem(names); i++)
    {
        Qid child = qid;
        if (walk(&child, (char *)names[i]))
        {
            continue;
        }
        Dir meta;
        uchar record[512];
        statfile(child, &meta);
        meta.name = (char *)names[i];
        int size = convD2M(&meta, record, sizeof(record));
        if (size <= BIT16SZ)
        {
            return Ebadarg;
        }
        if (position >= offset)
        {
            if ((ulong)size > *count-used)
            {
                if (!used)
                {
                    return Ebadarg;
                }
                break;
            }
            memmove(bytes+used, record, size);
            used += size;
        }
        position += size;
    }
    (void)id;
    *count = used;
    return nil;
}


/* EN/RU: Read held file identity or enumerate its live directory membership.
 * Прочитать held file identity либо перечислить live directory membership. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    int id = identity(qid);
    if (qid.type & QTDIR)
    {
        return directory(qid, bytes, count, offset);
    }
    if (id == 12 && denied)
    {
        return Eperm;
    }
    char value = id == 12 ? 'A' : id == 22 ? 'B' : id == 13 ? 'N' : id == 23 ? 'M' : id == 14 ? 'V' : 'S';
    *count = offset || !*count ? 0 : 1;
    if (*count)
    {
        *bytes = value;
    }
    return nil;
}


/* EN/RU: Fixture control mutates lookup/policy state only; no custom publication protocol is added.
 * Fixture control меняет только lookup/policy state; custom publication protocol не добавляется. */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (identity(qid) != 90 || offset)
    {
        return Eperm;
    }
    if (*count == 7 && !memcmp(bytes, "replace", 7))
    {
        replaced = 1;
    }
    else if (*count == 8 && !memcmp(bytes, "swap-sub", 8))
    {
        swapped = 1;
    }
    else if (*count == 3 && !memcmp(bytes, "add", 3))
    {
        added = 1;
    }
    else if (*count == 4 && !memcmp(bytes, "hide", 4))
    {
        hidden = 1;
    }
    else if (*count == 4 && !memcmp(bytes, "deny", 4))
    {
        denied = 1;
    }
    else if (*count == 7 && !memcmp(bytes, "restore", 7))
    {
        hidden = denied = 0;
    }
    else if (*count == 4 && !memcmp(bytes, "stop", 4))
    {
        stopping = 1;
    }
    else
    {
        return Ebadarg;
    }
    return nil;
}


/* EN/RU: Original POSIX socket owner serves all independent fids until explicit fixture shutdown.
 * Original POSIX socket owner обслуживает independent fids до explicit fixture shutdown. */
int main(void)
{
    Styxops ops = {0};
    struct sockaddr_in address;
    socklen_t size = sizeof(address);
    ops.walk = walk;
    ops.open = openfile;
    ops.read = readfile;
    ops.stat = statfile;
    ops.write = writefile;
    if (styxinit(&server, &ops, "0", 0555, 0) || getsockname(server.connfd, (struct sockaddr *)&address, &size))
    {
        return 2;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        if (styxwait(&server) || styxprocess(&server))
        {
            return 3;
        }
    }
    if (styxend(&server))
    {
        return 4;
    }
    print("PASS original Styx held subroot fids and live mutations\n");
    return 0;
}
