/* Native debugger-resource adapter; only its owner callback touches the borrowed VM context. */
#include "flashvm/rdebug.h"
#include "rdebug_format.h"
#include <stdlib.h>
#include <string.h>


/* Native ownership does not make the context thread-safe or retain any managed object. */
struct FVM_RDebug
{
    FVM_Context *context;                      /* Borrowed until successful destroy; never sent to an observer. */
    FVM_RDispatch *dispatch;                   /* Borrowed synchronized command/event owner. */
    FVM_RHandle resource;                      /* Registered resource incarnation. */
    FVM_RDebugService service;                 /* Host blocking/event service invoked at a VM stop. */
    void *userdata;                            /* Borrowed service state. */
    FVM_RDebugSnapshot snapshot;               /* Latest copied stop, retained after resume. */
    FVM_RDebugFrame frames[FVM_RDEBUG_FRAMES]; /* Cached independent frame metadata. */
    int active, resume;                        /* Synchronous callback lifetime and accepted resume intent. */
};


/* Canonical unsigned S integers preserve the user's compact big-endian wire profile. */
static size_t put_field(unsigned char *p, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, p, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}


/* Cursor updates stay private until the whole request or reply is valid. */
static int get_field(const unsigned char **p, size_t *left, unsigned long *value)
{
    FVM_SInteger number;
    size_t used;
    if (fvm_sinteger_read(*p, *left, &number, &used) != FVM_S_OK || number.high || number.negative)
    {
        return 0;
    }
    *p += used;
    *left -= used;
    *value = number.low;
    return 1;
}


/* Copy a trusted native C string into bounded storage and report explicit truncation. */
static int label(char *out, size_t bound, const char *input)
{
    size_t n = 0;
    if (input)
    {
        while (n < bound && input[n])
        {
            out[n] = input[n];
            ++n;
        }
    }
    out[n] = 0;
    return input && input[n] != 0;
}


/* Ten integer fields precede the counted function/source byte strings. */
static size_t snapshot_write(const FVM_RDebugSnapshot *s, unsigned char *out)
{
    size_t n = 0, function = strlen(s->function), source = strlen(s->source);
    n += put_field(out + n, FVM_RDEBUG_VERSION);
    n += put_field(out + n, s->state);
    n += put_field(out + n, s->flags);
    n += put_field(out + n, s->stop);
    n += put_field(out + n, s->offset);
    n += put_field(out + n, s->line);
    n += put_field(out + n, s->frames);
    n += put_field(out + n, s->controller);
    n += put_field(out + n, (unsigned long)function);
    n += put_field(out + n, (unsigned long)source);
    memcpy(out + n, s->function, function);
    n += function;
    memcpy(out + n, s->source, source);
    return n + source;
}


/* Frames omit bytecode/machine-code pointers and live managed values. */
static size_t frame_write(const FVM_RDebugFrame *f, unsigned char *out)
{
    size_t n = 0, length = strlen(f->function);
    n += put_field(out + n, FVM_RDEBUG_VERSION);
    n += put_field(out + n, f->truncated);
    n += put_field(out + n, f->engine);
    n += put_field(out + n, f->forced);
    n += put_field(out + n, f->offset);
    n += put_field(out + n, f->argc);
    n += put_field(out + n, f->registers);
    n += put_field(out + n, (unsigned long)length);
    memcpy(out + n, f->function, length);
    return n + length;
}


/* Strict decoder publishes no partial native snapshot on malformed input. */
FVM_RResult fvm_rdebug_snapshot_read(const void *raw, size_t size, FVM_RDebugSnapshot *out)
{
    const unsigned char *p = (const unsigned char *)raw;
    size_t left = size, i;
    unsigned long fields[FVM_RDEBUG_SNAPSHOT_FIELDS];
    FVM_RDebugSnapshot s;
    if (!p || !out)
    {
        return FVM_R_INVALID;
    }
    for (i = 0; i < FVM_RDEBUG_SNAPSHOT_FIELDS; ++i)
    {
        if (!get_field(&p, &left, fields + i))
        {
            return FVM_R_INVALID;
        }
    }
    if (fields[RDEBUG_VERSION] != FVM_RDEBUG_VERSION || fields[RDEBUG_SNAP_STATE] > FVM_RDEBUG_ABORTED ||
        (fields[RDEBUG_SNAP_FLAGS] & ~FVM_RDEBUG_FLAGS) || fields[RDEBUG_SNAP_FRAMES] > FVM_RDEBUG_FRAMES ||
        fields[RDEBUG_SNAP_FUNCTION] > FVM_RDEBUG_NAME || fields[RDEBUG_SNAP_SOURCE] > FVM_RDEBUG_SOURCE ||
        left != fields[RDEBUG_SNAP_FUNCTION] + fields[RDEBUG_SNAP_SOURCE] || memchr(p, 0, left) ||
        (fields[RDEBUG_SNAP_STATE] != FVM_RDEBUG_RUNNING && !fields[RDEBUG_SNAP_STOP]))
    {
        return FVM_R_INVALID;
    }
    memset(&s, 0, sizeof(s));
    s.state = (unsigned int)fields[RDEBUG_SNAP_STATE];
    s.flags = (unsigned int)fields[RDEBUG_SNAP_FLAGS];
    s.stop = fields[RDEBUG_SNAP_STOP];
    s.offset = fields[RDEBUG_SNAP_OFFSET];
    s.line = fields[RDEBUG_SNAP_LINE];
    s.frames = fields[RDEBUG_SNAP_FRAMES];
    s.controller = fields[RDEBUG_SNAP_CONTROLLER];
    memcpy(s.function, p, (size_t)fields[RDEBUG_SNAP_FUNCTION]);
    memcpy(s.source, p + fields[RDEBUG_SNAP_FUNCTION], (size_t)fields[RDEBUG_SNAP_SOURCE]);
    *out = s;
    return FVM_R_OK;
}


/* Decode a copied frame with canonical integers and an exact counted label. */
FVM_RResult fvm_rdebug_frame_read(const void *raw, size_t size, FVM_RDebugFrame *out)
{
    const unsigned char *p = (const unsigned char *)raw;
    size_t left = size, i;
    unsigned long fields[FVM_RDEBUG_FRAME_FIELDS];
    FVM_RDebugFrame f;
    if (!p || !out)
    {
        return FVM_R_INVALID;
    }
    for (i = 0; i < FVM_RDEBUG_FRAME_FIELDS; ++i)
    {
        if (!get_field(&p, &left, fields + i))
        {
            return FVM_R_INVALID;
        }
    }
    if (fields[RDEBUG_VERSION] != FVM_RDEBUG_VERSION || fields[RDEBUG_FRAME_TRUNCATED] > 1 ||
        fields[RDEBUG_FRAME_ENGINE] > FVM_CRASH_ENGINE_NATIVE || fields[RDEBUG_FRAME_FORCED] > 1 ||
        fields[RDEBUG_FRAME_FUNCTION] > FVM_RDEBUG_NAME || left != fields[RDEBUG_FRAME_FUNCTION] || memchr(p, 0, left))
    {
        return FVM_R_INVALID;
    }
    memset(&f, 0, sizeof(f));
    f.truncated = (unsigned int)fields[RDEBUG_FRAME_TRUNCATED];
    f.engine = (unsigned int)fields[RDEBUG_FRAME_ENGINE];
    f.forced = (unsigned int)fields[RDEBUG_FRAME_FORCED];
    f.offset = fields[RDEBUG_FRAME_OFFSET];
    f.argc = fields[RDEBUG_FRAME_ARGC];
    f.registers = fields[RDEBUG_FRAME_REGISTERS];
    memcpy(f.function, p, left);
    *out = f;
    return FVM_R_OK;
}


/* Publish without holding a VM reference in any event queue. */
static FVM_RResult publish(FVM_RDebug *d, unsigned long kind)
{
    unsigned char bytes[FVM_R_BYTES];
    unsigned long sequence;
    size_t size = snapshot_write(&d->snapshot, bytes);
    return fvm_rdispatch_publish(d->dispatch, d->resource, kind, bytes, size, &sequence);
}


/* Immutable server operations and owner FIFO serialization arbitrate one active controller. */
static FVM_RResult command(const FVM_RRequest *q, void *output, size_t capacity, size_t *used, void *raw)
{
    FVM_RDebug *d = (FVM_RDebug *)raw;
    const unsigned char *p = (const unsigned char *)q->data;
    unsigned long index, expected, enabled, length;
    size_t left = q->size;
    FVM_Result status;
    char name[FVM_RDEBUG_NAME + 1];
    (void)capacity;
    *used = 0;
    if (q->operation == FVM_RDEBUG_SNAPSHOT)
    {
        if (q->size)
        {
            return FVM_R_INVALID;
        }
        *used = snapshot_write(&d->snapshot, (unsigned char *)output);
        return FVM_R_OK;
    }
    if (q->operation == FVM_RDEBUG_FRAME)
    {
        if (!get_field(&p, &left, &expected) || !get_field(&p, &left, &index) || left)
        {
            return FVM_R_INVALID;
        }
        if (!d->snapshot.stop || expected != d->snapshot.stop)
        {
            return FVM_R_CONFLICT;
        }
        if (index >= d->snapshot.frames)
        {
            return FVM_R_UNKNOWN;
        }
        *used = frame_write(d->frames + index, (unsigned char *)output);
        return FVM_R_OK;
    }
    if (q->operation == FVM_RDEBUG_CLAIM)
    {
        if (q->size)
        {
            return FVM_R_INVALID;
        }
        if (d->snapshot.controller && d->snapshot.controller != q->session)
        {
            return FVM_R_CONFLICT;
        }
        d->snapshot.controller = q->session;
        return FVM_R_OK;
    }
    if (d->snapshot.controller != q->session)
    {
        return FVM_R_DENIED;
    }
    if (q->operation == FVM_RDEBUG_RELEASE)
    {
        if (q->size)
        {
            return FVM_R_INVALID;
        }
        d->snapshot.controller = 0;
        return FVM_R_OK;
    }
    if (q->operation == FVM_RDEBUG_RESUME)
    {
        if (!get_field(&p, &left, &expected) || left)
        {
            return FVM_R_INVALID;
        }
        if (!d->active)
        {
            return FVM_R_BUSY;
        }
        if (expected != d->snapshot.stop || d->resume)
        {
            return FVM_R_CONFLICT;
        }
        d->resume = 1;
        return FVM_R_OK;
    }
    if (q->operation != FVM_RDEBUG_BREAKPOINT)
    {
        return FVM_R_UNKNOWN;
    }
    if (!get_field(&p, &left, &expected) || !get_field(&p, &left, &enabled) || !get_field(&p, &left, &length) ||
        enabled > 1 || length > FVM_RDEBUG_NAME || left != length || memchr(p, 0, left))
    {
        return FVM_R_INVALID;
    }
    memcpy(name, p, left);
    name[left] = 0;
    status = fvm_debug_set_breakpoint(d->context, name, expected, (int)enabled);
    if (status == FVM_OK)
    {
        return FVM_R_OK;
    }
    if (status == FVM_OUT_OF_MEMORY)
    {
        return FVM_R_NOMEM;
    }
    return status == FVM_INVALID_ARGUMENT ? FVM_R_INVALID : FVM_R_HANDLER;
}


/* Do not replace an existing VM debugger hook implicitly. */
FVM_RResult fvm_rdebug_create(FVM_Context *context,
                              FVM_RDispatch *dispatch,
                              const char *path,
                              FVM_RDebugService service,
                              void *userdata,
                              FVM_RDebug **out,
                              FVM_RHandle *resource)
{
    static const FVM_ROperation operations[] = {{FVM_RDEBUG_SNAPSHOT, FVM_R_READ},
                                                {FVM_RDEBUG_FRAME, FVM_R_READ},
                                                {FVM_RDEBUG_BREAKPOINT, FVM_R_CONTROL},
                                                {FVM_RDEBUG_CLAIM, FVM_R_CONTROL},
                                                {FVM_RDEBUG_RELEASE, FVM_R_CONTROL},
                                                {FVM_RDEBUG_RESUME, FVM_R_CONTROL}};
    FVM_RDebug *d;
    FVM_RResult status;
    if (!context || !dispatch || !service || !out || !resource)
    {
        return FVM_R_INVALID;
    }
    d = (FVM_RDebug *)calloc(1, sizeof(*d));
    if (!d)
    {
        return FVM_R_NOMEM;
    }
    d->context = context;
    d->dispatch = dispatch;
    d->service = service;
    d->userdata = userdata;
    status = fvm_rdispatch_register(
        dispatch, path, operations, sizeof(operations) / sizeof(operations[0]), command, d, &d->resource);
    if (status != FVM_R_OK)
    {
        free(d);
        return status;
    }
    *resource = d->resource;
    *out = d;
    return FVM_R_OK;
}


/* Revoke before allowing a new controller to acquire the claim. */
FVM_RResult fvm_rdebug_revoke(FVM_RDebug *d, unsigned long session)
{
    FVM_RResult status;
    if (!d)
    {
        return FVM_R_INVALID;
    }
    status = fvm_rdispatch_grant(d->dispatch, d->resource, session, 0);
    if (status == FVM_R_OK && d->snapshot.controller == session)
    {
        d->snapshot.controller = 0;
    }
    return status;
}


/* Capture only during the legitimate synchronous VM stop; no managed value escapes it. */
FVM_Result fvm_rdebug_breakpoint(FVM_Context *context, const FVM_DebugBreakpoint *point, void *raw)
{
    FVM_RDebug *d = (FVM_RDebug *)raw;
    FVM_Result status = FVM_ERROR;
    size_t frames, i, done;
    if (!d || !point || d->context != context || d->active)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (d->snapshot.stop == FVM_SRECORD_U32 || point->offset > FVM_SRECORD_U32 || point->line > FVM_SRECORD_U32)
    {
        return FVM_UNSUPPORTED;
    }
    if (fvm_debug_frame_count(context, &frames) != FVM_OK)
    {
        return FVM_ERROR;
    }
    ++d->snapshot.stop;
    d->snapshot.state = FVM_RDEBUG_STOPPED;
    d->snapshot.flags = 0;
    d->snapshot.offset = point->offset;
    d->snapshot.line = point->line;
    if (label(d->snapshot.function, FVM_RDEBUG_NAME, point->function))
    {
        d->snapshot.flags |= FVM_RDEBUG_FUNCTION_TRUNCATED;
    }
    if (label(d->snapshot.source, FVM_RDEBUG_SOURCE, point->source))
    {
        d->snapshot.flags |= FVM_RDEBUG_SOURCE_TRUNCATED;
    }
    if (frames > FVM_RDEBUG_FRAMES)
    {
        frames = FVM_RDEBUG_FRAMES;
        d->snapshot.flags |= FVM_RDEBUG_FRAMES_TRUNCATED;
    }
    d->snapshot.frames = 0;
    for (i = 0; i < frames; ++i)
    {
        FVM_DebugFrame view;
        FVM_RDebugFrame *f = d->frames + i;
        size_t length;
        if (fvm_debug_frame_at(context, i, &view) != FVM_OK || view.offset > FVM_SRECORD_U32)
        {
            goto abort_stop;
        }
        memset(f, 0, sizeof(*f));
        f->offset = view.offset;
        f->argc = view.argc;
        f->registers = view.register_count;
        f->engine = (unsigned int)view.engine;
        f->forced = view.forced ? 1U : 0U;
        length = view.function_size;
        if (length > FVM_RDEBUG_NAME)
        {
            length = FVM_RDEBUG_NAME;
            f->truncated = 1;
        }
        if (length)
        {
            memcpy(f->function, view.function, length);
        }
        d->snapshot.frames = (unsigned long)i + 1;
    }
    d->active = 1;
    d->resume = 0;
    if (publish(d, FVM_RDEBUG_STOP_EVENT) != FVM_R_OK)
    {
        goto abort_stop;
    }
    while (!d->resume)
    {
        if (fvm_rdispatch_pump(d->dispatch, FVM_R_REQUESTS, &done) != FVM_R_OK)
        {
            goto abort_stop;
        }
        if (d->resume)
        {
            break;
        }
        status = d->service(d, d->userdata);
        if (status != FVM_OK)
        {
            goto abort_stop;
        }
    }
    d->active = 0;
    d->snapshot.state = FVM_RDEBUG_RUNNING;
    if (publish(d, FVM_RDEBUG_RESUME_EVENT) != FVM_R_OK)
    {
        status = FVM_ERROR;
        goto abort_stop;
    }
    return FVM_OK;
abort_stop:
    d->active = 0;
    d->snapshot.state = FVM_RDEBUG_ABORTED;
    publish(d, FVM_RDEBUG_ABORT_EVENT);
    return status == FVM_OK ? FVM_ERROR : status;
}


/* Context hook detachment is explicit so unrelated host debugger hooks are not overwritten. */
FVM_RResult fvm_rdebug_destroy(FVM_RDebug **pointer)
{
    FVM_RDebug *d;
    FVM_RResult status;
    if (!pointer || !*pointer)
    {
        return FVM_R_INVALID;
    }
    d = *pointer;
    if (d->active)
    {
        return FVM_R_BUSY;
    }
    status = fvm_rdispatch_unregister(d->dispatch, d->resource);
    if (status != FVM_R_OK)
    {
        return status;
    }
    free(d);
    *pointer = 0;
    return FVM_R_OK;
}
