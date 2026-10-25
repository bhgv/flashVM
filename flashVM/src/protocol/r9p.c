/* Original bounded 9P2000 adapter; transport framing is little-endian, file records remain S data.
 */
#include "flashvm/r9p.h"
#include "r9p_format.h"
#include <stdlib.h>
#include <string.h>


/* A parser cursor never publishes partially decoded fields into connection state. */
typedef struct R9Cursor
{
    const unsigned char *bytes; /* Borrow the complete validated input frame. */
    size_t left;                /* Bytes remaining inside that frame only. */
    int valid;                  /* Sticky bounds failure. */
} R9Cursor;


/* One fid belongs to one host-authenticated connection, not a user-provided uname. */
typedef struct R9Fid
{
    int active;                     /* Occupied wire fid. */
    unsigned long id;               /* Client-assigned Uint32 except NOFID. */
    FVM_RFSNode node;               /* Exact generation, never resolved again by registration name. */
    int opened;                     /* A successful open owns the file below. */
    FVM_RFSFile file;               /* Native descriptor borrowed from the connection's FS. */
    unsigned long directory_offset; /* Exact next encoded-directory byte offset. */
    size_t directory_index;         /* Next stable native directory entry. */
} R9Fid;


/* Only reads can wait; writes acknowledge native queue acceptance immediately. */
typedef struct R9Read
{
    int active;          /* This tag is outstanding until poll, flush or connection reset. */
    int closed;          /* The fid was clunked; poll returns an error without touching a replacement. */
    unsigned int tag;    /* Unique pending wire transaction tag. */
    size_t fid;          /* Index of the stable active fid slot. */
    unsigned long low;   /* Low offset half; no C99 integer extension is required. */
    unsigned long high;  /* High offset half. */
    unsigned long count; /* Requested bytes, clamped to negotiated reply size at execution. */
} R9Read;


/* Fixed connection state; the host supplies reliable ordered output and authentication. */
struct FVM_R9P
{
    FVM_RFS *fs;                /* Borrowed FS and its exclusive request/session namespace. */
    size_t msize;               /* Negotiated whole-frame bound, initially maximum. */
    int negotiated;             /* Only version can precede successful negotiation. */
    R9Fid fids[FVM_R9P_FIDS];   /* Bounded open and unopened references. */
    R9Read reads[FVM_R9P_FIDS]; /* At most one waiting read per fid. */
    size_t next_read;           /* Round-robin poll starting index; prevents hot-fid starvation. */
};


/* Read a fixed-width little-endian integer without native layout assumptions. */
static unsigned long r9_get(R9Cursor *c, size_t width)
{
    unsigned long value = 0;
    size_t i;
    if (!c->valid || c->left < width)
    {
        c->valid = 0;
        return 0;
    }
    for (i = 0; i < width; ++i)
    {
        value |= (unsigned long)c->bytes[i] << (i * R9_SHIFT);
    }
    c->bytes += width;
    c->left -= width;
    return value;
}


/* Borrow a counted string or byte span only if wholly inside the frame. */
static const unsigned char *r9_span(R9Cursor *c, size_t count)
{
    const unsigned char *p = c->bytes;
    if (!c->valid || count > c->left)
    {
        c->valid = 0;
        return 0;
    }
    c->bytes += count;
    c->left -= count;
    return p;
}


/* Validate a counted string; optionally make a bounded NUL-terminated native component. */
static int r9_string(R9Cursor *c, char *copy, size_t capacity, size_t *length)
{
    size_t count = (size_t)r9_get(c, R9_U16_BYTES);
    const unsigned char *p = r9_span(c, count);
    if (!c->valid || (copy && (count >= capacity || memchr(p, 0, count))))
    {
        c->valid = 0;
        return 0;
    }
    if (copy)
    {
        memcpy(copy, p, count);
        copy[count] = 0;
    }
    if (length)
    {
        *length = count;
    }
    return 1;
}


/* Encode little-endian integers bytewise; this never encodes an S-record metadata field. */
static void r9_put(unsigned char *out, unsigned long value, size_t width)
{
    size_t i;
    for (i = 0; i < width; ++i)
    {
        out[i] = (unsigned char)(value & R9_OCTET);
        value >>= R9_SHIFT;
    }
}


/* Append one counted string to a sufficiently bounded private output buffer. */
static size_t r9_text(unsigned char *out, const char *text)
{
    size_t n = strlen(text);
    r9_put(out, (unsigned long)n, R9_U16_BYTES);
    memcpy(out + R9_U16_BYTES, text, n);
    return R9_U16_BYTES + n;
}


/* Encode stable copied identity; content version is zero for these synthetic device files. */
static void r9_qid(unsigned char *out, const FVM_RFSEntry *entry)
{
    out[0] = entry->directory ? R9_QTDIR : 0;
    r9_put(out + 1, 0, R9_U32_BYTES);
    r9_put(out + 1 + R9_U32_BYTES, entry->qid_low, R9_U32_BYTES);
    r9_put(out + 1 + R9_U32_BYTES * 2, entry->qid_high, R9_U32_BYTES);
}


/* Fill the common reply header after a complete bounded payload has been prepared. */
static size_t r9_finish(unsigned char *out, unsigned int type, unsigned int tag, size_t size)
{
    r9_put(out, (unsigned long)size, R9_U32_BYTES);
    out[R9_U32_BYTES] = (unsigned char)type;
    r9_put(out + R9_U32_BYTES + 1, tag, R9_U16_BYTES);
    return size;
}


/* Map native service status to advisory protocol errors, never expose host addresses. */
static const char *r9_error_text(FVM_RResult result)
{
    switch (result)
    {
    case FVM_R_INVALID:
        return "invalid request";
    case FVM_R_LIMIT:
        return "resource limit";
    case FVM_R_UNKNOWN:
        return "unknown resource or fid";
    case FVM_R_DENIED:
        return "permission denied";
    case FVM_R_CONFLICT:
        return "stale snapshot or conflicting request";
    case FVM_R_BUSY:
        return "resource busy";
    case FVM_R_NOMEM:
        return "allocation failed";
    default:
        return "resource handler failure";
    }
}


/* Every error fits the minimum negotiated frame capacity without truncating a file entry. */
static size_t r9_error(unsigned char *out, unsigned int tag, FVM_RResult result)
{
    size_t n = r9_text(out + R9_HEADER, r9_error_text(result));
    return r9_finish(out, R9_ERROR, tag, R9_HEADER + n);
}


/* Find a current fid, including fid zero; NOFID is never attachable. */
static R9Fid *r9_fid(FVM_R9P *server, unsigned long id)
{
    size_t i;
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (server->fids[i].active && server->fids[i].id == id)
        {
            return server->fids + i;
        }
    }
    return 0;
}


/* Locate unused bounded storage without publishing client identity yet. */
static R9Fid *r9_free_fid(FVM_R9P *server)
{
    size_t i;
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (!server->fids[i].active && !server->fids[i].opened)
        {
            return server->fids + i;
        }
    }
    return 0;
}


/* Pending tags cannot be reused until a reply is handed to the host or a flush is answered. */
static int r9_tag(FVM_R9P *server, unsigned int tag)
{
    size_t i;
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (server->reads[i].active && server->reads[i].tag == tag)
        {
            return 1;
        }
    }
    return 0;
}


/* A second simultaneous stream read would make byte offsets/order ambiguous. */
static int r9_waiting(FVM_R9P *server, R9Fid *fid)
{
    size_t i, index = (size_t)(fid - server->fids);
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (server->reads[i].active && !server->reads[i].closed && server->reads[i].fid == index)
        {
            return 1;
        }
    }
    return 0;
}


/* Build the standard stat entry, including its inner size, without a native Dir dependency. */
static size_t r9_stat(unsigned char *out, const FVM_RFSEntry *entry)
{
    size_t n = R9_U16_BYTES;
    unsigned long mode;
    r9_put(out + n, 0, R9_U16_BYTES);
    n += R9_U16_BYTES;
    r9_put(out + n, 0, R9_U32_BYTES);
    n += R9_U32_BYTES;
    r9_qid(out + n, entry);
    n += R9_QID;
    mode = entry->directory ? R9_DMDIR | R9_DIR_MODE : entry->mode & FVM_RFS_WRITE ? R9_CALL_MODE : R9_FILE_MODE;
    r9_put(out + n, mode, R9_U32_BYTES);
    n += R9_U32_BYTES;
    /* Synthetic files expose no wall-clock timestamps or persistent backing-file lengths. */
    memset(out + n, 0, R9_U32_BYTES * 4);
    n += R9_U32_BYTES * 4;
    n += r9_text(out + n, entry->name);
    n += r9_text(out + n, "host");
    n += r9_text(out + n, "host");
    n += r9_text(out + n, "host");
    r9_put(out, (unsigned long)(n - R9_U16_BYTES), R9_U16_BYTES);
    return n;
}


/* Execute a read or leave it pending. Directory output always contains whole stat records. */
static FVM_RResult r9_read(FVM_R9P *server, R9Read *read, unsigned char *out, size_t *size)
{
    R9Fid *fid = server->fids + read->fid;
    FVM_RFSEntry entry;
    unsigned char stat[R9_STAT_MAX];
    size_t count, used = 0, n, index;
    unsigned long offset;
    FVM_RResult result;
    if (read->closed)
    {
        *size = r9_error(out, read->tag, FVM_R_UNKNOWN);
        return FVM_R_OK;
    }
    result = fvm_rfs_stat(server->fs, fid->node, &entry);
    if (result != FVM_R_OK)
    {
        *size = r9_error(out, read->tag, result);
        return FVM_R_OK;
    }
    count = (size_t)(read->count > server->msize - R9_READ_HEADER ? server->msize - R9_READ_HEADER : read->count);
    if (entry.directory)
    {
        if (read->high || (read->low && read->low != fid->directory_offset))
        {
            result = FVM_R_INVALID;
        }
        else
        {
            index = read->low ? fid->directory_index : 0;
            offset = read->low;
            /* Readdir validates discovery even for a zero-length read and at EOF. */
            for (;;)
            {
                result = fvm_rfs_readdir(server->fs, fid->file, index, &entry);
                if (result == FVM_R_PENDING)
                {
                    result = FVM_R_OK;
                    break;
                }
                if (result != FVM_R_OK)
                {
                    break;
                }
                n = r9_stat(stat, &entry);
                if (n > count - used)
                {
                    result = !used && count ? FVM_R_LIMIT : FVM_R_OK;
                    break;
                }
                if (offset > FVM_SRECORD_U32 - (unsigned long)n)
                {
                    result = FVM_R_LIMIT;
                    break;
                }
                memcpy(out + R9_READ_HEADER + used, stat, n);
                used += n;
                offset += (unsigned long)n;
                ++index;
            }
            if (result == FVM_R_OK)
            {
                fid->directory_index = index;
                fid->directory_offset = offset;
            }
        }
    }
    else if (read->high)
    {
        result = fid->node.kind == FVM_RFS_PATH ? FVM_R_OK : FVM_R_LIMIT;
    }
    else if (!count)
    {
        result = FVM_R_OK;
    }
    else
    {
        result = fvm_rfs_read(server->fs, fid->file, read->low, out + R9_READ_HEADER, count, &used);
    }
    if (result == FVM_R_PENDING)
    {
        return result;
    }
    if (result != FVM_R_OK)
    {
        *size = r9_error(out, read->tag, result);
    }
    else
    {
        r9_put(out + R9_HEADER, (unsigned long)used, R9_U32_BYTES);
        *size = r9_finish(out, R9_READ + 1, read->tag, R9_READ_HEADER + used);
    }
    return FVM_R_OK;
}


/* Reset/destroy cleanup makes bounded progress; the host must quiesce running owner callbacks. */
static FVM_RResult r9_close_all(FVM_R9P *server)
{
    size_t i;
    FVM_RResult result;
    memset(server->reads, 0, sizeof(server->reads));
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (server->fids[i].active || server->fids[i].opened)
        {
            if (server->fids[i].opened)
            {
                result = fvm_rfs_close(server->fs, server->fids[i].file);
                if (result != FVM_R_OK)
                {
                    return result;
                }
            }
            memset(server->fids + i, 0, sizeof(server->fids[i]));
        }
    }
    return FVM_R_OK;
}


/* Create one transport-independent connection without granting any authority. */
FVM_RResult fvm_r9p_create(FVM_RFS *fs, FVM_R9P **out)
{
    FVM_R9P *server;
    if (!fs || !out)
    {
        return FVM_R_INVALID;
    }
    server = (FVM_R9P *)calloc(1, sizeof(*server));
    if (!server)
    {
        return FVM_R_NOMEM;
    }
    server->fs = fs;
    server->msize = FVM_R9P_MSIZE;
    *out = server;
    return FVM_R_OK;
}


/* Decode complete request syntax before mutating fids, queues, subscriptions or offsets. */
static size_t r9_handle(FVM_R9P *server, unsigned int type, unsigned int tag, R9Cursor c, unsigned char *out)
{
    unsigned long fid_id, new_id, value, high, count;
    unsigned int mode, oldtag;
    R9Fid *fid, *target;
    FVM_RFSEntry entry;
    FVM_RFSNode node;
    FVM_RResult result = FVM_R_OK;
    R9Read read;
    size_t i, walked, n, used, version_size, names_count;
    char version[FVM_RFS_NAME + 1], names[FVM_R9P_WELEM][FVM_RFS_NAME + 1];
    const unsigned char *data;
    unsigned char qids[FVM_R9P_WELEM * R9_QID];
    if (type == R9_VERSION)
    {
        value = r9_get(&c, R9_U32_BYTES);
        r9_string(&c, version, sizeof(version), &version_size);
        if (!c.valid || c.left || tag != FVM_R9P_NOTAG || value < FVM_R9P_MIN_MSIZE)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        result = r9_close_all(server);
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        server->msize = value < FVM_R9P_MSIZE ? (size_t)value : FVM_R9P_MSIZE;
        server->negotiated = !strcmp(version, "9P2000") || (version_size >= sizeof("9P2000.") - 1 &&
                                                            !memcmp(version, "9P2000.", sizeof("9P2000.") - 1));
        r9_put(out + R9_HEADER, (unsigned long)server->msize, R9_U32_BYTES);
        n = r9_text(out + R9_HEADER + R9_U32_BYTES, server->negotiated ? "9P2000" : "unknown");
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_U32_BYTES + n);
    }
    if (type == R9_FLUSH)
    {
        oldtag = (unsigned int)r9_get(&c, R9_U16_BYTES);
        /* Malformed flush is a connection framing error handled before this function. */
        for (i = 0; i < FVM_R9P_FIDS; ++i)
        {
            if (server->reads[i].active && server->reads[i].tag == oldtag)
            {
                server->reads[i].active = 0;
            }
        }
        return r9_finish(out, type + 1, tag, R9_HEADER);
    }
    if (!server->negotiated || tag == FVM_R9P_NOTAG)
    {
        return r9_error(out, tag, FVM_R_INVALID);
    }
    if (type == R9_ATTACH)
    {
        fid_id = r9_get(&c, R9_U32_BYTES);
        value = r9_get(&c, R9_U32_BYTES);
        r9_string(&c, 0, 0, 0);
        r9_string(&c, 0, 0, &n);
        if (!c.valid || c.left || fid_id == FVM_R9P_NOFID)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        if (value != FVM_R9P_NOFID || n)
        {
            return r9_error(out, tag, FVM_R_DENIED);
        }
        if (r9_fid(server, fid_id))
        {
            return r9_error(out, tag, FVM_R_CONFLICT);
        }
        target = r9_free_fid(server);
        if (!target)
        {
            return r9_error(out, tag, FVM_R_LIMIT);
        }
        memset(&node, 0, sizeof(node));
        result = fvm_rfs_stat(server->fs, node, &entry);
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        memset(target, 0, sizeof(*target));
        target->active = 1;
        target->id = fid_id;
        target->node = node;
        r9_qid(out + R9_HEADER, &entry);
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_QID);
    }
    if (type == R9_AUTH)
    {
        return r9_error(out, tag, FVM_R_DENIED);
    }
    if (type == R9_WALK)
    {
        fid_id = r9_get(&c, R9_U32_BYTES);
        new_id = r9_get(&c, R9_U32_BYTES);
        names_count = (size_t)r9_get(&c, R9_U16_BYTES);
        if (names_count > FVM_R9P_WELEM)
        {
            return r9_error(out, tag, FVM_R_LIMIT);
        }
        for (i = 0; i < names_count; ++i)
        {
            r9_string(&c, names[i], sizeof(names[i]), 0);
        }
        if (!c.valid || c.left || new_id == FVM_R9P_NOFID)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        fid = r9_fid(server, fid_id);
        if (!fid)
        {
            return r9_error(out, tag, FVM_R_UNKNOWN);
        }
        if (fid->opened)
        {
            return r9_error(out, tag, FVM_R_BUSY);
        }
        target = fid_id == new_id ? fid : r9_fid(server, new_id);
        if (target && target != fid)
        {
            return r9_error(out, tag, FVM_R_CONFLICT);
        }
        if (!target)
        {
            target = r9_free_fid(server);
        }
        if (!target)
        {
            return r9_error(out, tag, FVM_R_LIMIT);
        }
        if (R9_HEADER + R9_U16_BYTES + names_count * R9_QID > server->msize)
        {
            return r9_error(out, tag, FVM_R_LIMIT);
        }
        node = fid->node;
        walked = 0;
        result = fvm_rfs_stat(server->fs, node, &entry);
        while (result == FVM_R_OK && walked < names_count)
        {
            result = fvm_rfs_walk(server->fs, node, names[walked], &entry);
            if (result == FVM_R_OK)
            {
                r9_qid(qids + walked * R9_QID, &entry);
                node = entry.node;
                ++walked;
            }
        }
        if (result != FVM_R_OK && !walked)
        {
            return r9_error(out, tag, result);
        }
        /* A partial walk returns qids but does not create or change newfid. */
        if (walked == names_count)
        {
            memset(target, 0, sizeof(*target));
            target->active = 1;
            target->id = new_id;
            target->node = node;
        }
        r9_put(out + R9_HEADER, (unsigned long)walked, R9_U16_BYTES);
        if (walked)
        {
            memcpy(out + R9_HEADER + R9_U16_BYTES, qids, walked * R9_QID);
        }
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_U16_BYTES + walked * R9_QID);
    }
    fid_id = r9_get(&c, R9_U32_BYTES);
    fid = r9_fid(server, fid_id);
    if (!c.valid)
    {
        return r9_error(out, tag, FVM_R_INVALID);
    }
    if (!fid)
    {
        return r9_error(out, tag, FVM_R_UNKNOWN);
    }
    if (type == R9_OPEN)
    {
        mode = (unsigned int)r9_get(&c, 1);
        if (!c.valid || c.left)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        if (fid->opened)
        {
            return r9_error(out, tag, FVM_R_BUSY);
        }
        if (mode != R9_OREAD && mode != R9_ORDWR)
        {
            return r9_error(out, tag, FVM_R_DENIED);
        }
        result = fvm_rfs_stat(server->fs, fid->node, &entry);
        if (result == FVM_R_OK)
        {
            result = fvm_rfs_open(
                server->fs, fid->node, mode == R9_OREAD ? FVM_RFS_READ : FVM_RFS_READ | FVM_RFS_WRITE, &fid->file);
        }
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        fid->opened = 1;
        r9_qid(out + R9_HEADER, &entry);
        value = server->msize - R9_IOHDRSZ;
        if (value > FVM_R_BYTES)
        {
            value = FVM_R_BYTES;
        }
        r9_put(out + R9_HEADER + R9_QID, value, R9_U32_BYTES);
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_QID + R9_U32_BYTES);
    }
    if (type == R9_STAT)
    {
        if (c.left)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        result = fvm_rfs_stat(server->fs, fid->node, &entry);
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        n = r9_stat(out + R9_HEADER + R9_U16_BYTES, &entry);
        r9_put(out + R9_HEADER, (unsigned long)n, R9_U16_BYTES);
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_U16_BYTES + n);
    }
    if (type == R9_CLUNK)
    {
        if (c.left)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        for (i = 0; i < FVM_R9P_FIDS; ++i)
        {
            if (server->reads[i].active && server->reads[i].fid == (size_t)(fid - server->fids))
            {
                server->reads[i].closed = 1;
            }
        }
        if (fid->opened)
        {
            result = fvm_rfs_close(server->fs, fid->file);
        }
        /* 9P clunk invalidates the wire fid even on error. Retain only busy native cleanup. */
        fid->active = 0;
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        memset(fid, 0, sizeof(*fid));
        return r9_finish(out, type + 1, tag, R9_HEADER);
    }
    if (type != R9_READ && type != R9_WRITE)
    {
        return r9_error(out, tag, FVM_R_DENIED);
    }
    value = r9_get(&c, R9_U32_BYTES);
    high = r9_get(&c, R9_U32_BYTES);
    count = r9_get(&c, R9_U32_BYTES);
    if (!fid->opened)
    {
        return r9_error(out, tag, FVM_R_DENIED);
    }
    if (type == R9_WRITE)
    {
        data = r9_span(&c, (size_t)count);
        if (!c.valid || c.left || high)
        {
            return r9_error(out, tag, FVM_R_INVALID);
        }
        result = fvm_rfs_write(server->fs, fid->file, value, data, (size_t)count, &used);
        if (result != FVM_R_OK)
        {
            return r9_error(out, tag, result);
        }
        r9_put(out + R9_HEADER, (unsigned long)used, R9_U32_BYTES);
        return r9_finish(out, type + 1, tag, R9_HEADER + R9_U32_BYTES);
    }
    if (!c.valid || c.left)
    {
        return r9_error(out, tag, FVM_R_INVALID);
    }
    if (r9_waiting(server, fid))
    {
        return r9_error(out, tag, FVM_R_BUSY);
    }
    memset(&read, 0, sizeof(read));
    read.active = 1;
    read.tag = tag;
    read.fid = (size_t)(fid - server->fids);
    read.low = value;
    read.high = high;
    read.count = count;
    result = r9_read(server, &read, out, &n);
    if (result == FVM_R_OK)
    {
        return n;
    }
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (!server->reads[i].active)
        {
            server->reads[i] = read;
            return 0;
        }
    }
    return r9_error(out, tag, FVM_R_LIMIT);
}


/* Process one validated frame. Mutating protocol operations cannot fail on output capacity later.
 */
FVM_RResult fvm_r9p_request(
    FVM_R9P *server, const void *input, size_t size, void *output, size_t capacity, size_t *used, size_t *consumed)
{
    R9Cursor c;
    unsigned long length;
    unsigned int type, tag;
    size_t n;
    if (!server || (!input && size) || !output || !used || !consumed)
    {
        return FVM_R_INVALID;
    }
    if (capacity < FVM_R9P_MSIZE)
    {
        return FVM_R_LIMIT;
    }
    if (size < R9_U32_BYTES)
    {
        return FVM_R_PENDING;
    }
    c.bytes = (const unsigned char *)input;
    c.left = size;
    c.valid = 1;
    length = r9_get(&c, R9_U32_BYTES);
    if (length < R9_HEADER)
    {
        return FVM_R_INVALID;
    }
    if (length > FVM_R9P_MSIZE || (server->negotiated && length > server->msize))
    {
        return FVM_R_LIMIT;
    }
    if (size < length)
    {
        return FVM_R_PENDING;
    }
    c.left = (size_t)length - R9_U32_BYTES;
    type = (unsigned int)r9_get(&c, 1);
    tag = (unsigned int)r9_get(&c, R9_U16_BYTES);
    if (r9_tag(server, tag))
    {
        return FVM_R_CONFLICT;
    }
    /* A malformed flush must never generate Rerror, so reject its frame at the native boundary. */
    if (type == R9_FLUSH && c.left != R9_U16_BYTES)
    {
        return FVM_R_INVALID;
    }
    n = r9_handle(server, type, tag, c, (unsigned char *)output);
    *used = n;
    *consumed = (size_t)length;
    return FVM_R_OK;
}


/* Poll round-robin so continuously ready events cannot starve a quieter pending request. */
FVM_RResult fvm_r9p_poll(FVM_R9P *server, void *output, size_t capacity, size_t *used)
{
    size_t i, index, n;
    FVM_RResult result;
    if (!server || !output || !used)
    {
        return FVM_R_INVALID;
    }
    if (capacity < FVM_R9P_MSIZE)
    {
        return FVM_R_LIMIT;
    }
    /* A clunked fid may still own a request whose owner callback was running at close. */
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        if (!server->fids[i].active && server->fids[i].opened)
        {
            result = fvm_rfs_close(server->fs, server->fids[i].file);
            if (result == FVM_R_OK)
            {
                memset(server->fids + i, 0, sizeof(server->fids[i]));
            }
            else if (result != FVM_R_BUSY)
            {
                return result;
            }
        }
    }
    for (i = 0; i < FVM_R9P_FIDS; ++i)
    {
        index = (server->next_read + i) % FVM_R9P_FIDS;
        if (!server->reads[index].active)
        {
            continue;
        }
        result = r9_read(server, server->reads + index, (unsigned char *)output, &n);
        if (result == FVM_R_PENDING)
        {
            continue;
        }
        if (result != FVM_R_OK)
        {
            return result;
        }
        server->reads[index].active = 0;
        server->next_read = (index + 1) % FVM_R9P_FIDS;
        *used = n;
        return FVM_R_OK;
    }
    return FVM_R_PENDING;
}


/* Release owned fids before ending the borrowed FS lifetime; callers quiesce their transport. */
FVM_RResult fvm_r9p_destroy(FVM_R9P **pointer)
{
    FVM_R9P *server;
    FVM_RResult result;
    if (!pointer || !(server = *pointer))
    {
        return FVM_R_INVALID;
    }
    result = r9_close_all(server);
    if (result != FVM_R_OK)
    {
        return result;
    }
    free(server);
    *pointer = 0;
    return FVM_R_OK;
}
