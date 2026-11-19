/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of convS2M.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

        p += BIT32SZ;
        break;

    case Twrite:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT64(p, f->offset);
        p += BIT64SZ;
        PBIT32(p, f->count);
        p += BIT32SZ;
        if (f->count != 0)
        {
            memmove(p, f->data, f->count);
        }
        p += f->count;
        break;

    case Tclunk:
    case Tremove:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        break;

    case Tstat:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        break;

    case Twstat:
        PBIT32(p, f->fid);
        p += BIT32SZ;
        PBIT16(p, f->nstat);
        p += BIT16SZ;
        if (f->nstat != 0)
        {
            memmove(p, f->stat, f->nstat);
        }
        p += f->nstat;
        break;
        /*
 */

    case Rversion:
        PBIT32(p, f->msize);
        p += BIT32SZ;
        p = pstring(p, f->version);
        break;

    case Rerror:
        p = pstring(p, f->ename);
        break;

    case Rflush:
        break;

    case Rauth:
        p = pqid(p, &f->aqid);
        break;

    case Rattach:
        p = pqid(p, &f->qid);
        break;

    case Rwalk:
        PBIT16(p, f->nwqid);
        p += BIT16SZ;
        if (f->nwqid > MAXWELEM)
        {
            return 0;
        }
        for (i = 0; i < f->nwqid; i++)
        {
            p = pqid(p, &f->wqid[i]);
        }
        break;

    case Ropen:
    case Rcreate:
        p = pqid(p, &f->qid);
        PBIT32(p, f->iounit);
        p += BIT32SZ;
        break;

    case Rread:
        PBIT32(p, f->count);
        p += BIT32SZ;
        if (f->count != 0)
        {
            memmove(p, f->data, f->count);
        }
        p += f->count;
        break;

    case Rwrite:
        PBIT32(p, f->count);
        p += BIT32SZ;
        break;

    case Rclunk:
        break;

    case Rremove:
        break;

    case Rstat:
        PBIT16(p, f->nstat);
        p += BIT16SZ;
        if (f->nstat != 0)
        {
            memmove(p, f->stat, f->nstat);
        }
        p += f->nstat;
        break;

    case Rwstat:
        break;
    }
    if (size != p - ap)
    {
        return 0;
    }
    return size;
}
