/* Independent standard 9P2000 client bytes for live player fixtures; transport is in-memory here.
 */
#include "resource_9p_client.h"
#include <stdio.h>
#include <string.h>


static void put(unsigned char *p, unsigned long n, size_t width)
{
    size_t i;
    for (i = 0; i < width; ++i)
    {
        p[i] = (unsigned char)(n & 255);
        n >>= 8;
    }
}


static unsigned long get(const unsigned char *p, size_t width)
{
    size_t i;
    unsigned long n = 0;
    for (i = 0; i < width; ++i)
    {
        n |= (unsigned long)p[i] << (i * 8);
    }
    return n;
}


static size_t text(unsigned char *p, const char *s)
{
    size_t n = strlen(s);
    put(p, (unsigned long)n, 2);
    memcpy(p + 2, s, n);
    return n + 2;
}


static int send(Test9PClient *c, unsigned int type, unsigned int tag, const void *data, size_t size)
{
    unsigned char bytes[FVM_R9P_MSIZE];
    size_t consumed;
    if (size > sizeof(bytes) - 7)
    {
        return 0;
    }
    put(bytes, (unsigned long)size + 7, 4);
    bytes[4] = (unsigned char)type;
    put(bytes + 5, tag, 2);
    if (size)
    {
        memcpy(bytes + 7, data, size);
    }
    return fvm_r9p_request(c->server, bytes, size + 7, c->output, sizeof(c->output), &c->size, &consumed) == FVM_R_OK &&
           consumed == size + 7;
}


static int response(Test9PClient *c, unsigned int type, unsigned int tag)
{
    return c->size <= c->msize && c->size >= 7 && get(c->output, 4) == c->size && c->output[4] == type &&
           get(c->output + 5, 2) == tag;
}


static int walk(Test9PClient *c, unsigned long fid, FVM_RHandle h, const char *middle, const char *leaf)
{
    unsigned char bytes[128];
    char name[32];
    size_t n = 10;
    sprintf(name, "%lu.%lu", h.id, h.generation);
    put(bytes, 0, 4);
    put(bytes + 4, fid, 4);
    put(bytes + 8, leaf ? 3 : 2, 2);
    n += text(bytes + n, name);
    n += text(bytes + n, middle);
    if (leaf)
    {
        n += text(bytes + n, leaf);
    }
    return send(c, 110, 2, bytes, n) && response(c, 111, 2) && get(c->output + 7, 2) == (unsigned long)(leaf ? 3 : 2);
}


static int open_file(Test9PClient *c, unsigned long fid, unsigned int mode)
{
    unsigned char bytes[5];
    put(bytes, fid, 4);
    bytes[4] = (unsigned char)mode;
    return send(c, 112, 3, bytes, sizeof(bytes)) && response(c, 113, 3);
}


static int read_file(Test9PClient *c, unsigned long fid, unsigned long offset, unsigned int tag)
{
    unsigned char bytes[16];
    memset(bytes, 0, sizeof(bytes));
    put(bytes, fid, 4);
    put(bytes + 4, offset, 4);
    put(bytes + 12, FVM_R9P_MSIZE, 4);
    return send(c, 116, tag, bytes, sizeof(bytes));
}


int test_9p_create(Test9PClient *c, FVM_RDispatch *d, unsigned long session, FVM_RHandle target)
{
    return test_9p_create_sized(c, d, session, target, FVM_R9P_MSIZE);
}


/* Negotiate an explicit limit before attach; the server may lower but never raise it. */
int test_9p_create_sized(Test9PClient *c, FVM_RDispatch *d, unsigned long session, FVM_RHandle target, size_t msize)
{
    unsigned char bytes[64];
    size_t n;
    c->msize = msize;
    if (msize < FVM_R9P_MIN_MSIZE || msize > FVM_R9P_MSIZE)
    {
        return 0;
    }
    if (fvm_rfs_create(d, session, &c->fs) != FVM_R_OK || fvm_r9p_create(c->fs, &c->server) != FVM_R_OK)
    {
        return 0;
    }
    put(bytes, (unsigned long)msize, 4);
    n = 4 + text(bytes + 4, "9P2000");
    if (!send(c, 100, 65535, bytes, n) || !response(c, 101, 65535) || c->size < 13 || get(c->output + 7, 4) > msize ||
        get(c->output + 7, 4) < FVM_R9P_MIN_MSIZE)
    {
        return 0;
    }
    c->msize = (size_t)get(c->output + 7, 4);
    put(bytes, 0, 4);
    put(bytes + 4, 0xffffffffUL, 4);
    n = 8 + text(bytes + 8, "untrusted-wire-name");
    n += text(bytes + n, "");
    return send(c, 104, 1, bytes, n) && response(c, 105, 1) && walk(c, 1, target, "events", 0) && open_file(c, 1, 0) &&
           fvm_sbindings_init(&c->bindings, &c->slot, 1) == FVM_S_OK;
}


int test_9p_command(Test9PClient *c, FVM_RHandle target, unsigned long operation)
{
    unsigned char bytes[16];
    char name[16];
    sprintf(name, "%lu", operation);
    if (!walk(c, 2, target, "ops", name) || !open_file(c, 2, 2))
    {
        return 0;
    }
    memset(bytes, 0, sizeof(bytes));
    put(bytes, 2, 4);
    return send(c, 118, 4, bytes, sizeof(bytes)) && response(c, 119, 4) && get(c->output + 7, 4) == 0;
}


/* Read to file EOF: one S-record can span multiple negotiated-size Rread messages. */
static int reply_payload(Test9PClient *c, FVM_RReply *reply)
{
    FVM_SBindings bindings;
    FVM_SBinding slot;
    unsigned long id;
    size_t used, n, total = 0;
    unsigned char clunk[4], data[FVM_RFS_BUFFER];
    do
    {
        if (!read_file(c, 2, (unsigned long)total, 5) || !response(c, 117, 5) || c->size < 11 ||
            get(c->output + 7, 4) != c->size - 11)
        {
            return 0;
        }
        n = c->size - 11;
        if (n > sizeof(data) - total)
        {
            return 0;
        }
        memcpy(data + total, c->output + 11, n);
        total += n;
    } while (n);
    if (fvm_sbindings_init(&bindings, &slot, 1) != FVM_S_OK ||
        fvm_sbindings_apply(&bindings, data, total, &used) != FVM_S_OK)
    {
        return 0;
    }
    n = used;
    if (fvm_rreply_record_read(&bindings, data + n, total - n, &id, reply, &used) != FVM_S_OK || used != total - n ||
        reply->result != FVM_R_OK)
    {
        return 0;
    }
    put(clunk, 2, 4);
    return send(c, 120, 6, clunk, sizeof(clunk)) && response(c, 121, 6);
}


int test_9p_reply(Test9PClient *c)
{
    FVM_RReply reply;
    return reply_payload(c, &reply) && !reply.size;
}


int test_9p_call(Test9PClient *c,
                 FVM_RDispatch *dispatch,
                 FVM_RHandle target,
                 unsigned long operation,
                 const void *payload,
                 size_t size,
                 FVM_RReply *reply)
{
    unsigned char bytes[FVM_R_BYTES + 16];
    char name[16];
    size_t done;
    if (size > FVM_R_BYTES || (!payload && size))
    {
        return 0;
    }
    sprintf(name, "%lu", operation);
    if (!walk(c, 2, target, "ops", name) || !open_file(c, 2, 2))
    {
        return 0;
    }
    memset(bytes, 0, 16);
    put(bytes, 2, 4);
    put(bytes + 12, (unsigned long)size, 4);
    if (size)
    {
        memcpy(bytes + 16, payload, size);
    }
    return send(c, 118, 4, bytes, size + 16) && response(c, 119, 4) && get(c->output + 7, 4) == size &&
           fvm_rdispatch_pump(dispatch, 1, &done) == FVM_R_OK && done == 1 && reply_payload(c, reply);
}


int test_9p_query(Test9PClient *c, FVM_RDispatch *dispatch, FVM_RHandle target, FVM_RReply *reply)
{
    size_t done;
    return test_9p_command(c, target, 1) && fvm_rdispatch_pump(dispatch, 1, &done) == FVM_R_OK && done == 1 &&
           reply_payload(c, reply);
}


FVM_RResult test_9p_event(Test9PClient *c, FVM_REvent *event)
{
    FVM_RResult result;
    size_t used, n;
    const unsigned char *p;
    if (!c->pending)
    {
        if (!read_file(c, 1, c->offset, 100))
        {
            return FVM_R_INVALID;
        }
        c->pending = c->size == 0;
    }
    if (c->pending)
    {
        result = fvm_r9p_poll(c->server, c->output, sizeof(c->output), &c->size);
        if (result != FVM_R_OK)
        {
            return result;
        }
        c->pending = 0;
    }
    if (!response(c, 117, 100) || c->size < 11 || get(c->output + 7, 4) != c->size - 11)
    {
        return FVM_R_INVALID;
    }
    n = c->size - 11;
    p = c->output + 11;
    if (!c->offset)
    {
        if (fvm_sbindings_apply(&c->bindings, p, n, &used) != FVM_S_OK)
        {
            return FVM_R_INVALID;
        }
        p += used;
        n -= used;
    }
    c->offset += (unsigned long)c->size - 11;
    return fvm_revent_record_read(&c->bindings, p, n, event, &used) == FVM_S_OK && used == n ? FVM_R_OK : FVM_R_INVALID;
}


int test_9p_destroy(Test9PClient *c)
{
    int ok = 1;
    if (c->server && fvm_r9p_destroy(&c->server) != FVM_R_OK)
    {
        ok = 0;
    }
    if (!c->server && c->fs && fvm_rfs_destroy(&c->fs) != FVM_R_OK)
    {
        ok = 0;
    }
    return ok;
}
