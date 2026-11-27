/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of styxserver.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
            {

                fprint(2, " %s", f.wname[i]);
            }

            fprint(2, "\n");
        }
        if (c->attach != nil && c->attach->fid == f.newfid)
        {
            f.type = Rerror;
            f.ename = "fid reserved by pending attach";
            wr(c, &f);
            break;
        }
        nfp = findfid(c, f.newfid);
        f.type = Rerror;
        if (nfp != nil && f.newfid != f.fid)
        {
            f.ename = "fid in use";
            if (Debug)
            {
                fprint(2, "walk: %s\n", f.ename);
            }
            wr(c, &f);
            break;
        }
        else if (fp->open)
        {
            f.ename = "can't clone";
            wr(c, &f);
            break;
        }
        if (f.newfid != f.fid)
        {
            nfp = fidclone(fp, f.newfid);
        }
        else
        {
            nfp = fp;
        }
        if ((wq = devwalk(c, file, fp, nfp, f.wname, f.nwname, &f.ename)) == nil)
        {
            if (nfp != fp)
            {
                deletefid(c, nfp);
            }
            f.type = Rerror;
        }
        else
        {
            if (nfp != fp)
            {
                if (wq->nqid != f.nwname)
                {
                    deletefid(c, nfp);
                }
            }
            f.type = Rwalk;
            f.nwqid = wq->nqid;
            for (i = 0; i < wq->nqid; i++)
            {
                f.wqid[i] = wq->qid[i];
            }
            styxfree(wq);
        }
        wr(c, &f);
        break;
    case Topen:
        if (Debug)
        {
            fprint(2, "Topen %d\n", f.fid);
        }
        f.ename = nil;
        if (fp->open)
        {
            f.ename = Eopen;
        }
        else if ((fp->qid.type & QTDIR) && (f.mode & (OWRITE | OTRUNC | ORCLOSE)))
        {
            f.ename = Eperm;
        }
        else if (file != nil && !styxperm(file, c->uname, f.mode))
        {
            f.ename = Eperm;
        }
        else if ((f.mode & ORCLOSE) && file != nil && file->parent != nil && !styxperm(file->parent, c->uname, OWRITE))
        {
            f.ename = Eperm;
        }
        if (f.ename != nil)
        {
            f.type = Rerror;
            wr(c, &f);
            break;
        }
        f.ename = Enonexist;
        decreff(fp);
        if (ops->open == nil || (f.ename = ops->open(&fp->qid, f.mode)) == nil)
        {
            f.type = Ropen;
            f.qid = fp->qid;
            fp->mode = f.mode;
            fp->open = 1;
            fp->offset = 0;
            incopen(fp);
        }
        else
        {
            f.type = Rerror;
        }
        increff(fp);
        wr(c, &f);
        break;
    case Tcreate:
        if (Debug)
        {
            fprint(2, "Tcreate %d %s\n", f.fid, f.name);
        }
        f.ename = nil;
        if (fp->open)
        {
            f.ename = Eopen;
        }
        else if (!(fp->qid.type & QTDIR))
        {
            f.ename = Enotdir;
        }
        else if ((f.perm & DMDIR) && (f.mode & (OWRITE | OTRUNC | ORCLOSE)))
        {
            f.ename = Eperm;
        }
        else if (file != nil && !styxperm(file, c->uname, OWRITE))
        {
            f.ename = Eperm;
        }
        if (f.ename != nil)
        {
            f.type = Rerror;
            wr(c, &f);
            break;
        }
        f.ename = Eperm;
        decreff(fp);
        if (file != nil)
        {
            if (f.perm & DMDIR)
            {
                f.perm = (f.perm & ~0777) | (file->d.mode & f.perm & 0777) | DMDIR;
            }
            else
            {
                f.perm = (f.perm & (~0777 | 0111)) | (file->d.mode & f.perm & 0666);
            }
        }
        if (ops->create && (f.ename = ops->create(&fp->qid, f.name, f.perm, f.mode)) == nil)
        {
            f.type = Rcreate;
            f.qid = fp->qid;
            fp->mode = f.mode;
            fp->open = 1;
            fp->offset = 0;
            incopen(fp);
        }
        else
        {
            f.type = Rerror;
        }
        increff(fp);
        wr(c, &f);
        break;
    case Tread:
        if (Debug)
        {
            fprint(2, "Tread %d\n", f.fid);
        }
        if (!fp->open || (fp->mode & 3) == OWRITE)
        {
            f.type = Rerror;
            f.ename = Ebadfid;
            wr(c, &f);
            break;
        }
        if (f.count > sizeof(c->data) - IOHDRSZ)
        {
            f.count = sizeof(c->data) - IOHDRSZ;
        }
        if (c->msize != 0 && f.count > c->msize - IOHDRSZ)
        {
            f.count = c->msize - IOHDRSZ;
        }
        count = f.count;
        isdir = (fp->qid.type & QTDIR) || (file != nil && (file->d.qid.type & QTDIR));
        f.type = Rerror;
        f.ename = Eperm;
        if (isdir && file != nil && (c->server->needfile || ops->read == nil))
        {
            if (f.offset == 0)
            {
                fp->dri = 0;
                fp->offset = 0;
            }
            if (f.offset != fp->offset)
            {
                f.ename = "invalid directory offset";
            }
            else
            {
                dircount = devdirread(fp, file, c->data, f.count);
                if (dircount < 0)
                {
                    f.ename = "directory entry exceeds read capacity";
                }
                else
                {
                    count = dircount;
                    fp->offset += count;
                    f.type = Rread;
                }
            }
        }
        else if (ops->read != nil)
        {
            if (callbackdata(c, fp, &f, &count, isdir, Tread))
            {
                break;
            }
            if (f.ename == nil)
            {
                f.type = Rread;
            }
        }
        if (f.type == Rread)
        {
            if (count > f.count)
            {
                f.type = Rerror;
                f.ename = "invalid read count";
            }
            else if (isdir && !validdir(c->data, count))
            {
                f.type = Rerror;
                f.ename = "invalid directory records";
            }
            else
            {
                f.count = count;
                f.data = c->data;
            }
        }
        wr(c, &f);
        break;
    case Twrite:
        if (Debug)
        {
            fprint(2, "Twrite %d\n", f.fid);
        }
        if (!fp->open || ((fp->mode & 3) != OWRITE && (fp->mode & 3) != ORDWR))
        {
            f.type = Rerror;
            f.ename = Ebadfid;
            wr(c, &f);
            break;
        }
        f.ename = Eperm;
        f.type = Rerror;
        count = f.count;
        isdir = (fp->qid.type & QTDIR) || (file != nil && (file->d.qid.type & QTDIR));
        if (ops->write && callbackdata(c, fp, &f, &count, isdir, Twrite))
        {
            break;
        }
        if (f.ename == nil)
        {
            if (count > f.count)
            {
                f.ename = "invalid write count";
            }
            else
            {
                f.count = count;
                f.type = Rwrite;
            }
        }
        wr(c, &f);
        break;
    case Tclunk:
        if (Debug)
        {
            fprint(2, "Tclunk %d\n", f.fid);
        }
        open = fp->open;
        mode = fp->mode;
        qid = fp->qid;
        deletefid(c, fp);
        f.type = Rclunk;
        if (open && ops->close && (f.ename = ops->close(qid, mode)) != nil)
        {
            f.type = Rerror;
        }
        wr(c, &f);
        break;
    case Tremove:
        if (Debug)
        {
            fprint(2, "Tremove %d\n", f.fid);
        }
        if (file != nil && file->parent != nil && !styxperm(file->parent, c->uname, OWRITE))
        {
            f.type = Rerror;
            f.ename = Eperm;
            deletefid(c, fp);
            wr(c, &f);
            break;
        }
        f.ename = Eperm;
        if (ops->remove && (f.ename = ops->remove(fp->qid)) == nil)
        {
            f.type = Rremove;
        }
        else
        {
            f.type = Rerror;
        }
        deletefid(c, fp);
        wr(c, &f);
        break;
    case Tstat:
        if (Debug)
        {
            fprint(2, "Tstat %d qid=%llx\n", f.fid, fp->qid.path);
        }
        memset(&dir, 0, sizeof(dir));
        f.stat = styxmalloc(MAXSTAT);
        f.ename = "stat error";
        if (ops->stat == nil && file != nil)
        {
            f.type = Rstat;
            f.nstat = convD2M(&file->d, f.stat, c->msize != 0 ? c->msize - 9 : MAXSTAT);
        }
        else if (ops->stat && (f.ename = ops->stat(fp->qid, &dir)) == nil)
        {
            f.type = Rstat;
            f.nstat = convD2M(&dir, f.stat, c->msize != 0 ? c->msize - 9 : MAXSTAT);
        }
        else
        {
            f.type = Rerror;
        }
        if (f.type == Rstat && f.nstat <= BIT16SZ)
        {
            f.type = Rerror;
            f.ename = "stat record exceeds message capacity";
        }
        wr(c, &f);
        styxfree(f.stat);
        break;
    case Twstat:
        if (Debug)
        {
            fprint(2, "Twstat %d\n", f.fid);
        }
        f.ename = Eperm;
        memset(&dir, 0, sizeof(dir));
        if (f.nstat < STATFIXLEN || convM2D(f.stat, f.nstat, &dir, strs) != f.nstat)
        {
            f.type = Rerror;
            f.ename = "invalid stat record";
            wr(c, &f);
            break;
        }
        dir.name = nilconv(dir.name);
        dir.uid = nilconv(dir.uid);
        dir.gid = nilconv(dir.gid);
        dir.muid = nilconv(dir.muid);
        if (ops->wstat && (f.ename = ops->wstat(fp->qid, &dir)) == nil)
        {
            f.type = Rwstat;
        }
        else
        {
            f.type = Rerror;
        }
        wr(c, &f);
        break;
    case Tversion:
        if (f.tag != NOTAG || f.msize < 256)
        {
            f.type = Rerror;
            f.ename = "invalid version tag or msize below 256";
            wr(c, &f);
            break;
        }
        cancelattach(c);
        cancelreads(c, nil);
        deletefids(c);
        c->msize = f.msize > MSGMAX ? MSGMAX : f.msize;
        c->versioned = strcmp(f.version, VERSION9P) == 0 || strncmp(f.version, VERSION9P ".", 7) == 0 ? 1 : -1;
        f.msize = c->msize;
        f.version = c->versioned > 0 ? VERSION9P : "unknown";
        f.type = Rversion;
        wr(c, &f);
        break;
    case Tauth:
        f.type = Rerror;
        f.ename = "authentication not provided";
        wr(c, &f);
        break;
    case Tattach:
        beginattach(c, &f);
        break;
    }
}

/* Current fid storage is borrowed only while a callback is executing. */


/* [standalone-contract:run]
 * EN: run: original server session/fid dispatch and callback-backed namespace operations.
 * RU: run: исходная диспетчеризация сессий/fid и callback-операции namespace.
 * c (Client *c): Original connection context, valid only while its owning server retains it.
 * RU: Исходный контекст соединения, действующий пока сервер его удерживает.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void run(Client *c)
{
    c->current = nil;
    dispatch(c);
    c->current = nil;
}

/* No implicit sharing or replacement of a native handle is allowed. */
