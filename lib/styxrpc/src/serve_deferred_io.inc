/* Copyright (c) 2026 bhgv
 * EN: Run opt-in file callbacks without holding the 9P dispatcher.
 * RU: Выполнять выбранные колбэки файлов, освобождая диспетчер 9P.
 */
#define TREE_IO_LIMIT 16U /* Per-server bound, including workers retiring after a flush. */


/* EN: Owner keeps the wire request and pins the file cookie through worker join.
 * RU: Владелец удерживает запрос протокола и состояние файла до завершения потока. */
typedef struct TreeIo
{
    struct TreeIo *next;   /* Owner-only linkage. */
    TreeFid *fid;          /* Exclusive file cookie, pinned past clunk if necessary. */
    StyxRead *request;     /* Native request reference, on the dispatcher only. */
    StyxPhyWake *wake;     /* Borrowed server wake pin through join. */
    pthread_t thread;      /* Worker joined by the owner. */
    uint64_t offset;       /* Original absolute byte position. */
    size_t capacity;       /* Original bounded request size. */
    size_t used;           /* Confirmed count, published with done. */
    int writing;           /* Bytes are input for a write and output for a read. */
    int started;           /* Owner must join when thread creation succeeded. */
    int done;              /* Worker publishes completion atomically. */
    int cancelled;         /* Owner publishes retirement atomically. */
    StyxRpcStatus status;  /* Provider outcome, published with done. */
    unsigned char bytes[]; /* Owned buffer, independent of server frame storage. */
} TreeIo;

static __thread TreeIo *worker_io; /* Borrowed only during the current file callback. */


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_io_cancelled(void)
{
    return worker_io && __atomic_load_n(&worker_io->cancelled, __ATOMIC_ACQUIRE);
}


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_enable_deferred_io(StyxRpcServer *server, StyxRpcDeferFile select)
{
    if (!server || !select || server->started || server->defer_file ||
        !ordinary_tree(server))
    {
        return 0;
    }
    server->io_wake = styxphywakepin(&server->wire);
    if (!server->io_wake)
    {
        return 0;
    }
    server->defer_file = select;
    return 1;
}


/* EN: Set the file's deferred access mask from the owner selector. The fid
 * and its installed cookie remain borrowed. Other adapters never enter it.
 * RU: Установить маску отложенного доступа через выбор владельца.
 * Fid и установленное состояние заимствованы. Для других адаптеров выбора нет. */
static void select_deferred(StyxRpcServer *server, TreeFid *fid, uint64_t id)
{
    if (server->defer_file && fid->file.ops == &server->tree->files)
    {
        fid->deferred_access = fid->file.access & server->defer_file(server->context, id);
    }
}


/* EN: Drop the request reference and signal a parked callback when its 9P
 * lease retires. The owner keeps io and its cookie until worker join.
 * RU: Освободить ссылку на запрос и сообщить ожидающему колбэку о снятии
 * запроса 9P. Владелец сохраняет io и состояние до завершения потока. */
static void retire_io(void *state)
{
    TreeIo *io = state;
    if (io->writing)
    {
        styxwritefree(io->request);
    }
    else
    {
        styxreadfree(io->request);
    }
    io->request = NULL;
    __atomic_store_n(&io->cancelled, 1, __ATOMIC_RELEASE);
}


/* EN: Invoke the selected callback once, sharing only the cancel flag and
 * publishing the result before waking the owner. Borrow state through join.
 * RU: Вызвать выбранный колбэк один раз. Общий только флаг остановки.
 * Опубликовать результат перед пробуждением владельца. State живёт до join. */
static void *run_io(void *state)
{
    TreeIo *io = state;
    worker_io = io;
    if (!styxrpc_server_io_cancelled())
    {
        if (io->writing)
        {
            io->status = styxrpc_file_write(&io->fid->file, io->offset, io->bytes, io->capacity, &io->used);
        }
        else
        {
            io->status = styxrpc_file_read(&io->fid->file, io->offset, io->bytes, io->capacity, &io->used);
        }
    }
    worker_io = NULL;
    __atomic_store_n(&io->done, 1, __ATOMIC_RELEASE);
    styxphywake(io->wake);
    return NULL;
}


/* EN: Admit a nonempty transfer on fid with one native request slot. Borrow
 * server/fid, copy write input, and clear count only when the worker starts.
 * RU: Принять непустую передачу fid в один слот протокола. Заимствовать
 * server/fid, скопировать запись и обнулить count только при запуске потока. */
static char *defer_file_io(StyxRpcServer *server, TreeFid *fid, const void *input,
    ulong *count, uint64_t offset)
{
    if (server->io_count >= TREE_IO_LIMIT || *count > SIZE_MAX - sizeof(TreeIo))
    {
        return "too many pending data requests";
    }
    TreeIo *io = calloc(1, sizeof(*io) + *count);
    if (!io)
    {
        return Enomem;
    }
    io->fid = fid;
    io->offset = offset;
    io->capacity = *count;
    io->wake = server->io_wake;
    io->writing = input != NULL;
    io->status = STYXRPC_INVALID;
    io->request = io->writing ? styxdeferwrite(&server->wire, io, retire_io) :
        styxdeferread(&server->wire, io, retire_io);
    if (!io->request)
    {
        free(io);
        return Enomem;
    }
    if (input)
    {
        memcpy(io->bytes, input, *count);
    }
    io->next = server->pending_io;
    server->pending_io = io;
    ++server->io_count;
    fid->io = io;
    if (pthread_create(&io->thread, NULL, run_io, io))
    {
        __atomic_store_n(&io->done, 1, __ATOMIC_RELEASE);
        return Enomem;
    }
    io->started = 1;
    *count = 0;
    return nil;
}


/* EN: Publish finished requests when the transport can accept a reply, or
 * join retired workers after wire teardown. Close a retired cookie on this owner.
 * RU: Выдать результаты, когда транспорт может принять ответ, либо дождаться
 * потоков после закрытия протокола. Освободить закрытый fid у владельца. */
static void finish_io(StyxRpcServer *server, int closing)
{
    TreeIo **link = &server->pending_io;
    while (*link)
    {
        TreeIo *io = *link;
        if (!closing && (!__atomic_load_n(&io->done, __ATOMIC_ACQUIRE) ||
            (io->request && !styxphyspace(&server->wire))))
        {
            link = &io->next;
            continue;
        }
        if (!closing && io->request)
        {
            char *error = io->status == STYXRPC_OK ? nil : Ebadarg;
            ulong count = error ? 0 : io->used;
            int replied = io->writing ? styxwritereply(io->request, count, error) :
                styxreadreply(io->request, io->bytes, count, error);
            if (!replied)
            {
                link = &io->next;
                continue;
            }
        }
        if (io->started)
        {
            pthread_join(io->thread, NULL);
        }
        io->fid->io = NULL;
        if (io->fid->retired)
        {
            close_tree_fid(io->fid);
        }
        *link = io->next;
        --server->io_count;
        free(io);
    }
}
