/* Copyright (c) 2026 bhgv
 * EN: Hosted system entry points and fallback stubs; included once by main.c.
 * RU: Системные host-входы и резервные заглушки; однократно подключается из main.c.
 */


/* Preserve formatted VM errors before unwinding their current owner. */
void errorf(char *format, ...)
{
    char text[256];
    va_list arguments;
    va_start(arguments, format);
    vsnprint(text, sizeof(text), format, arguments);
    va_end(arguments);
    error(text);
}


/* Dynamic native code is excluded; ordinary Dis never enters this boundary. */
Module *newdyncode(int fd, char *path, Dir *metadata)
{
    USED(fd);
    USED(path);
    USED(metadata);
    error("native library loading unsupported");
    return nil;
}


/* Reject native data attachment rather than manufacturing a successful import. */
void newdyndata(Modlink *module)
{
    USED(module);
    error("native data loading unsupported");
}


/* There are no native dynamic-code allocations in this host. */
void freedyncode(Module *module)
{
    USED(module);
}


/* There are no native dynamic-data allocations in this host. */
void freedyndata(Modlink *module)
{
    USED(module);
}


/* Ask the host allocator about metadata storage; VM blocks use poolmsize instead. */
ulong msize(void *value)
{


    extern size_t malloc_usable_size(void *);
    return malloc_usable_size(value);
}


/* Validate language path contents before converting to a native NUL-terminated name.
 * name is borrowed; returns the UTF-8 path or NULL with a visible error. */
char *dvm_runtime_file_path(String *name)
{
    int index;
    if (name == H || name == nil)
    {
        error(exNilref);
    }
    if (name->len == (-0x7fffffff - 1))
    {
        error(exBounds);
    }
    if (name->len >= 0)
    {
        if (memchr(name->Sascii, 0, name->len))
        {
            kwerrstr("file paths cannot contain NUL");
            return nil;
        }
    }
    else
    {
        for (index = 0; index < -name->len; index++)
        {
            if (name->Srune[index] == 0)
            {
                kwerrstr("file paths cannot contain NUL");
                return nil;
            }
        }
    }
    return string2c(name);
}


/* Open ordinary read-only input, or the explicitly enabled isolated-test control node. */
void Sys_open(void *frame)
{
    F_Sys_open local;
    F_Sys_open *call = &local;
    Sys_FD *old;
    Sys_FD *result;
    int descriptor;
    char *path;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_open, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    path = dvm_runtime_file_path(call->s);
    if (!path)
    {
        Sys_FD *empty = H;
        memmove(call->ret, &empty, sizeof(empty));
        destroy(old);
        return;
    }
    if (call->s == H)
    {
        error(exNilref);
    }
    result = H2D(Sys_FD *, heapz(TFD));
    result->fd = -1;
    if (test_control && strcmp(path, "/dev/sysctl") == 0 && call->mode == 1)
    {
        descriptor = -2;
    }
    else
    {
        descriptor = -1;
        if (call->mode == OREAD)
        { /* [GC:] The unpublished FD is a C-held root across native waiting. */
            ptradd(D2H(result));
            Setmark(D2H(result));
            descriptor = open_file_mode(path, "r", 1);
            ptrdel(D2H(result));
        }
        else
        {
            kwerrstr("unsupported file open mode");
        }
    }
    if (descriptor >= 0 || descriptor == -2)
    {
        result->fd = descriptor;
    }
    else
    {
        destroy(result);
        result = H;
    }
    memmove(call->ret, &result, sizeof(result));
    destroy(old);
}


/* Write to actual native files/standard output, or consume the explicit test halt capability. */
void Sys_write(void *frame)
{
    F_Sys_write local;
    F_Sys_write *call = &local;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_write, ret), sizeof(*call->ret));
    if (test_control && call->fd != H && call->fd != nil && call->fd->fd == -2 && call->buf != H && call->buf != nil &&
        call->n == 4 && call->buf->len >= 4 && memcmp(call->buf->data, "halt", 4) == 0)
    {
        fflush(stdout);
        inspect_release(); /* [GC:] Explicit test halt also retires any captured scalar data before exit. */
        runtime_statistics();
        exit(0);
    }
    {
        WORD result = dvm_runtime_write(call->fd, call->buf, call->n);
        memmove(call->ret, &result, sizeof(result));
    }
}

/* Share the NC64 leaf policy with complete JIT tails and its standalone check. */
#include "native_leaf.c"


/* Publish a new descriptor while preserving the previous result owner until construction completes. */
static void publish_descriptor(void *slot, Sys_FD *result, Sys_FD *old)
{
    memmove(slot, &result, sizeof(result));
    destroy(old);
}


/* Borrow standard descriptors 0/1/2 without taking ownership of their OS handles. */
void Sys_fildes(void *frame)
{
    F_Sys_fildes local;
    F_Sys_fildes *call = &local;
    Sys_FD *result = H;
    Sys_FD *old;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_fildes, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    if (call->fd >= 0 && call->fd <= 2)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = call->fd;
    }
    else
    {
        kwerrstr("only standard descriptors 0, 1 and 2 can be borrowed");
    }
    publish_descriptor(call->ret, result, old);
}


/* Create/truncate a file only after validating flags and allocating its result owner.
 * The supported permission value is 0666 filtered by the platform umask. */
void Sys_create(void *frame)
{
    F_Sys_create local;
    F_Sys_create *call = &local;
    Sys_FD *result = H;
    Sys_FD *old;
    int descriptor;
    char *path;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_create, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    path = dvm_runtime_file_path(call->s);
    if (!path)
    {
        publish_descriptor(call->ret, H, old);
        return;
    }
    if (call->s == H || call->s == nil)
    {
        error(exNilref);
    }
    if ((call->mode == Sys_OWRITE || call->mode == (Sys_OWRITE | Sys_OEXCL)) && call->perm == 0666)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = -1;
        /* [GC:] Preserve the unpublished FD while other owners collect. */
        ptradd(D2H(result));
        Setmark(D2H(result));
        descriptor = open_file_mode(path, (call->mode & Sys_OEXCL) ? "wx" : "w", 1);
        ptrdel(D2H(result));
        if (descriptor < 0)
        {
            destroy(result);
            result = H;
        }
        else
        {
            result->fd = descriptor;
        }
    }
    else
    {
        kwerrstr("create supports OWRITE, optional OEXCL and permission 0666 only");
    }
    publish_descriptor(call->ret, result, old);
}
