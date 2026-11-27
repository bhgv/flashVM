/* Copyright (c) 2026 bhgv
 * EN: Gate exact 9P control replies while a detached SDK caller polls cancellation.
 * RU: Удерживать точные ответы 9P, пока отдельный вызывающий SDK проверяет отмену.
 */
#include <lib9.h>
#include <styx.h>
#include "client.h"
#include "styxrpc/vfs_client.h"
#include "styxrpc/module_client.h"
#include "styxrpc/scalar_client.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define FRAME_LIMIT 512U /* Negotiated frame budget. */
#define FID_LIMIT 32U    /* Upper bound on the fixture's allocated fid numbers. */

typedef enum Operation
{
    PathStat,      /* Metadata on a temporary walked fid. */
    FileStat,      /* Metadata on a borrowed open fid. */
    FileOpen,      /* Open an ordinary file. */
    DirectoryOpen, /* Open an independently owned directory cursor. */
    DirectoryNext, /* Read one directory record. */
    Create,        /* Exclusive creation of an ordinary file. */
    MakeDirectory, /* Creation with DMDIR and read mode. */
    Remove,        /* Remove and consume a walked fid. */
    ModuleCapture, /* Clone a borrowed module directory. */
    ModuleExports, /* Read a catalog on that directory. */
    ModuleCheck,   /* Read the catalog for one declared import. */
    ScalarCall     /* Verify a scalar profile before writing any request bytes. */
} Operation;

typedef struct Scenario
{
    const char *name;    /* Diagnostic label. */
    Operation call;      /* Application entry point. */
    int frame;           /* Request to hold, or zero when stopped before I/O. */
    unsigned occurrence; /* Which occurrence to hold, beginning at one. */
    int reply_first;     /* Deliver the real reply before Rflush. */
    StyxRpcIoOutcome expected; /* Exact API status requested by this scenario. */
} Scenario;

typedef struct Peer
{
    const Scenario *scenario; /* Borrowed until both threads retire. */
    int fd;                   /* Owned stream endpoint. */
    int armed;                /* Atomic gate begins after setup. */
    int held;                 /* Atomic publication after saving exact reply and tag. */
    int stopped;              /* Atomic callback signal sent by the supervisor. */
    int clean;                /* EOF at a frame boundary with valid traffic. */
    unsigned selected;        /* Requests of the selected type after setup. */
    unsigned flushes;         /* Tflush count for the exact held tag. */
    unsigned controls;        /* Reads served through the independent fid while held. */
    unsigned armed_frames;    /* Non-cleanup, non-control requests during the tested operation. */
    unsigned char reply[FRAME_LIMIT]; /* Original acknowledgement, suspended by the fixture. */
    unsigned length;          /* Encoded reply length. */
    ushort tag;               /* Request tag retired by flush. */
    u32int survivor;          /* Control fid for one ordinary positive read. */
    Qid fids[FID_LIMIT];       /* Wire identities for the test's walked fids. */
} Peer;

typedef struct Call
{
    Peer *peer;             /* Borrowed fixture through join. */
    StyxClient *session;    /* Borrowed caller session. */
    StyxRpcVfsFile *file;   /* Owned input for FileStat or output for FileOpen/Create. */
    StyxRpcVfsDir *dir;     /* Owned input for DirectoryNext or output for DirectoryOpen. */
    StyxFid *module;        /* Owned fixture directory borrowed by the operation. */
    pthread_t owner;       /* Expected thread for the hook. */
    StyxRpcIoOutcome result; /* API outcome from the worker. */
    int valid;              /* Output and owner-thread verification. */
    unsigned visits;        /* Successful callbacks for this one directory step. */
} Call;


/* EN: Build one fixture metadata record for qid/name in the borrowed stat buffer.
 * RU: Закодировать одну запись qid/name в предоставленном буфере stat. */
static unsigned pack_stat(Qid qid, const char *name, uchar *stat, unsigned size)
{
    Dir info = {.qid = qid, .name = (char *)name, .uid = "owner", .gid = "owner",
        .muid = "owner", .mode = qid.type & QTDIR ? DMDIR | 0777 : 0666,
        .length = qid.type & QTDIR ? 0 : 1};
    return convD2M(&info, stat, size);
}


/* EN: Construct the response to a decoded request on the fixture owner.
 * Write stat/read bytes into borrowed extra, which remains valid through encoding.
 * RU: Составить ответ на запрос в потоке провайдера. extra хранит байты stat/read
 * до завершения кодирования. */
static int response(Peer *peer, const Fcall *request, Fcall *reply, uchar *extra)
{
    if (request->fid >= FID_LIMIT && request->type != Tversion && request->type != Tflush)
    {
        return 0;
    }
    switch (request->type)
    {
    case Tversion:
        reply->msize = FRAME_LIMIT;
        reply->version = VERSION9P;
        return 1;
    case Tattach:
        reply->qid = (Qid){.type = QTDIR, .path = 0};
        peer->fids[request->fid] = reply->qid;
        return 1;
    case Twalk:
        if (request->newfid >= FID_LIMIT)
        {
            return 0;
        }
        reply->nwqid = request->nwname;
        Qid current = peer->fids[request->fid];
        for (unsigned i = 0; i < request->nwname; ++i)
        {
            const char *name = request->wname[i];
            int directory = !strcmp(name, "d") || !strcmp(name, "dir");
            current = (Qid){.type = directory ? QTDIR : QTFILE, .path = directory ? 2 : 3};
            reply->wqid[i] = current;
        }
        peer->fids[request->newfid] = current;
        if (request->nwname == 1 && !strcmp(request->wname[0], "survivor"))
        {
            peer->survivor = request->newfid;
        }
        return 1;
    case Tstat:
        reply->stat = extra;
        reply->nstat = pack_stat(peer->fids[request->fid], "entry", extra, FRAME_LIMIT);
        return reply->nstat > 2;
    case Topen:
        reply->qid = peer->fids[request->fid];
        return 1;
    case Tcreate:
        if ((request->perm & DMDIR) ?
            (request->perm != (DMDIR | 0777U) || request->mode != OREAD || strcmp(request->name, "folder")) :
            (request->perm != 0666U || request->mode != OWRITE || strcmp(request->name, "new")))
        {
            return 0;
        }
        reply->qid = (Qid){.type = request->perm & DMDIR ? QTDIR : QTFILE, .path = 4};
        peer->fids[request->fid] = reply->qid;
        return 1;
    case Tread:
        if (peer->fids[request->fid].type & QTDIR)
        {
            reply->count = !request->offset ? pack_stat((Qid){.path = 5}, "entry", extra, FRAME_LIMIT) : 0;
            reply->data = (char *)extra;
        }
        else
        {
            reply->count = 1;
            reply->data = "q";
        }
        return reply->count <= request->count;
    case Tremove:
    case Tclunk:
    case Tflush:
        return 1;
    default:
        return 0;
    }
}


/* EN: Send a response unless it is the exact request selected by the test.
 * Tflush retires its original tag and optionally releases the earlier reply.
 * RU: Отправить ответ, кроме выбранного тестом запроса. Tflush завершает
 * исходный tag и при выбранном режиме выпускает прежнее подтверждение. */
static int answer(Peer *peer, const Fcall *request)
{
    unsigned char frame[FRAME_LIMIT], extra[FRAME_LIMIT];
    Fcall reply = {.type = request->type + 1, .tag = request->tag};
    if (!response(peer, request, &reply, extra))
    {
        return 0;
    }
    unsigned size = convS2M(&reply, frame, sizeof(frame));
    if (!size)
    {
        return 0;
    }
    if (request->type == Tflush)
    {
        if (!peer->held || request->oldtag != peer->tag || peer->flushes++ || peer->controls != 1)
        {
            return 0;
        }
        if (peer->scenario->reply_first &&
            send(peer->fd, peer->reply, peer->length, MSG_NOSIGNAL) != (ssize_t)peer->length)
        {
            return 0;
        }
    }
    else if (__atomic_load_n(&peer->armed, __ATOMIC_ACQUIRE))
    {
        if (request->type == Tread && request->fid == peer->survivor)
        {
            ++peer->controls;
        }
        else if (request->type != Tclunk)
        {
            ++peer->armed_frames;
            if (request->type == peer->scenario->frame &&
                ++peer->selected == peer->scenario->occurrence)
            {
                memcpy(peer->reply, frame, size);
                peer->length = size;
                peer->tag = request->tag;
                __atomic_store_n(&peer->held, 1, __ATOMIC_RELEASE);
                return 1;
            }
        }
    }
    return send(peer->fd, frame, size, MSG_NOSIGNAL) == (ssize_t)size;
}


/* EN: Own and close the provider endpoint, receiving complete original 9P frames.
 * RU: Владеть дескриптором провайдера, принимать полные кадры 9P и закрыть его. */
static void *serve(void *state)
{
    Peer *peer = state;
    uchar frame[FRAME_LIMIT];
    ssize_t received;
    while ((received = recv(peer->fd, frame, 4, MSG_WAITALL)) == 4)
    {
        Fcall request;
        unsigned size = GBIT32(frame);
        if (size < 7 || size > sizeof(frame) ||
            recv(peer->fd, frame + 4, size - 4, MSG_WAITALL) != (ssize_t)(size - 4) ||
            convM2S(frame, size, &request) != size || !answer(peer, &request))
        {
            break;
        }
    }
    peer->clean = received == 0;
    close(peer->fd);
    return NULL;
}


/* EN: Record a directory callback and validate its borrowed entry.
 * RU: Учесть колбэк каталога и проверить заимствованную запись. */
static int visit(void *state, const char *name, const StyxRpcFileInfo *info)
{
    Call *call = state;
    ++call->visits;
    return !strcmp(name, "entry") && !info->directory;
}


/* EN: Sample the supervisor's stop flag, checking execution stays on the caller.
 * RU: Проверить флаг остановки и убедиться, что колбэк остался в потоке вызова. */
static int stopped(void *state)
{
    Call *call = state;
    call->valid &= pthread_equal(call->owner, pthread_self()) != 0;
    return __atomic_load_n(&call->peer->stopped, __ATOMIC_ACQUIRE);
}


/* EN: Execute the selected public operation and check its output contract.
 * RU: Выполнить выбранный публичный вызов и проверить выходные значения. */
static void *run(void *state)
{
    Call *call = state;
    const Scenario *test = call->peer->scenario;
    StyxRpcFileInfo info = {99, 0777, 1};
    int found = 7;
    call->owner = pthread_self();
    call->valid = 1;
    switch (test->call)
    {
    case PathStat:
        call->result = styxrpc_vfs_stat_until(call->session, "file", &info, stopped, call);
        break;
    case FileStat:
        call->result = styxrpc_vfs_file_stat_until(call->file, &info, stopped, call);
        break;
    case FileOpen:
        call->result = styxrpc_vfs_open_until(call->session, test->occurrence == 2 ?
            "d/d/d/d/d/d/d/d/d/d/d/d/d/d/d/d/file" : "file",
            STYXRPC_FILE_READ, &call->file, stopped, call);
        call->valid &= !!call->file == (test->expected == STYXRPC_IO_OK);
        break;
    case DirectoryOpen:
        call->result = styxrpc_vfs_dir_open_until(call->session, "dir", &call->dir, stopped, call);
        call->valid &= !!call->dir == (test->expected == STYXRPC_IO_OK);
        break;
    case DirectoryNext:
        call->result = styxrpc_vfs_dir_next_until(call->dir, visit, call, &found, stopped, call);
        call->valid &= found == (test->expected == STYXRPC_IO_OK) &&
            call->visits == (unsigned)found;
        if (call->result == STYXRPC_IO_CANCELLED)
        {
            call->valid &= styxrpc_vfs_dir_rewind(call->dir) == STYXRPC_IO_NOT_OPEN;
        }
        break;
    case Create:
        call->result = styxrpc_vfs_create_until(call->session, "new", &call->file, stopped, call);
        call->valid &= !!call->file == (test->expected == STYXRPC_IO_OK);
        break;
    case MakeDirectory:
        call->result = styxrpc_vfs_mkdir_until(call->session, "folder", stopped, call);
        break;
    case Remove:
        call->result = styxrpc_vfs_remove_until(call->session, "file", stopped, call);
        break;
    case ModuleCapture:
    {
        StyxFid *captured = call->module;
        call->result = styxrpc_module_capture_until(NULL, NULL, call->module, &captured, stopped, call);
        call->valid &= captured == call->module;
        if (captured != call->module)
        {
            styxclientclunk(captured);
        }
        break;
    }
    case ModuleExports:
    {
        StyxRpcModuleExports exports = {.count = 99};
        uint32_t scopes = 17;
        StyxRpcStatus status = styxrpc_module_exports_scoped_at_until(call->module, 0,
            &exports, &scopes, stopped, call);
        call->result = status == STYXRPC_CANCELLED ? STYXRPC_IO_CANCELLED : STYXRPC_IO_FAILED;
        call->valid &= exports.count == 99 && scopes == 17;
        break;
    }
    case ModuleCheck:
    {
        const StyxRpcModuleImport import = {"echo", STYXRPC_MODULE_INT, STYXRPC_INT_SIGNATURE, {0}};
        StyxRpcStatus status = styxrpc_module_check_at_until(call->module, &import, 1, 0, stopped, call);
        call->result = status == STYXRPC_CANCELLED ? STYXRPC_IO_CANCELLED : STYXRPC_IO_FAILED;
        break;
    }
    case ScalarCall:
    {
        int64_t result = 73;
        StyxRpcOutcome status = styxrpc_scalar_invoke_at_until(call->module, NULL, NULL,
            "echo", STYXRPC_INT_SIGNATURE, -4, 1000, &result, stopped, call);
        call->result = status == STYXRPC_CALL_CANCELLED ? STYXRPC_IO_CANCELLED : STYXRPC_IO_FAILED;
        call->valid &= result == 73;
        break;
    }
    }
    if ((test->call == FileStat || test->call == PathStat) && test->expected != STYXRPC_IO_OK)
    {
        call->valid &= info.length == 99 && info.mode == 0777 && info.directory == 1;
    }
    return NULL;
}


/* EN: Check liveness on a borrowed file independent of the held operation.
 * RU: Проверить прогресс обмена через заимствованный файл, независимый от ожидания. */
static int read_control(StyxRpcVfsFile *file)
{
    char byte;
    size_t used;
    return styxrpc_vfs_file_read(file, 0, &byte, 1, &used) == STYXRPC_IO_OK && used == 1 && byte == 'q';
}


/* EN: Run one scenario on fresh endpoints; join every admitted worker and verify wire counts.
 * RU: Выполнить один сценарий на новых соединениях, дождаться потоков и проверить счётчики. */
static int exercise(const Scenario *test)
{
    int fd[2];
    pthread_t provider, caller;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
    {
        return 0;
    }
    Peer peer = {.scenario = test, .fd = fd[1], .stopped = !test->frame};
    if (pthread_create(&provider, NULL, serve, &peer))
    {
        close(fd[0]);
        close(fd[1]);
        return 0;
    }
    StyxClient *session = styxclientmountfd(fd[0], FRAME_LIMIT, "", "", 2500, 8);
    StyxRpcVfsFile *survivor = NULL;
    Call call = {.peer = &peer, .session = session};
    int setup = session && styxrpc_vfs_open(session, "survivor", STYXRPC_FILE_READ, &survivor) == STYXRPC_IO_OK &&
        (test->call != FileStat || styxrpc_vfs_open(session, "file", STYXRPC_FILE_READ, &call.file) == STYXRPC_IO_OK) &&
        (test->call != DirectoryNext || styxrpc_vfs_dir_open(session, "dir", &call.dir) == STYXRPC_IO_OK) &&
        (test->call < ModuleCapture || (call.module = styxrpc_module_open(session, "dir")) != NULL);
    __atomic_store_n(&peer.armed, 1, __ATOMIC_RELEASE);
    if (setup && !pthread_create(&caller, NULL, run, &call))
    {
        const struct timespec interval = {0, 1000000};
        unsigned remaining = 1400;
        if (test->frame)
        {
            while (remaining-- && !__atomic_load_n(&peer.held, __ATOMIC_ACQUIRE))
            {
                nanosleep(&interval, NULL);
            }
            setup = __atomic_load_n(&peer.held, __ATOMIC_ACQUIRE) && read_control(survivor);
            __atomic_store_n(&peer.stopped, 1, __ATOMIC_RELEASE);
        }
        pthread_join(caller, NULL);
    }
    else
    {
        setup = 0;
    }
    __atomic_store_n(&peer.armed, 0, __ATOMIC_RELEASE);
    if (setup && test->call == FileStat)
    {
        setup = read_control(call.file);
    }
    styxclientclunk(call.module);
    styxrpc_vfs_dir_close(call.dir, 0);
    styxrpc_vfs_close(call.file, 0);
    styxrpc_vfs_close(survivor, 0);
    styxclientshutdown(session);
    styxclientfree(session);
    pthread_join(provider, NULL);
    int passed = setup && call.valid && call.result == test->expected && peer.clean &&
        (test->frame ? peer.selected == test->occurrence && peer.flushes == 1 : !peer.armed_frames);
    if (!passed)
    {
        fprintf(stderr, "%s failed: status %d/%d, setup %d, output %d, frames %u, selected %u, flushes %u, clean %d\n",
            test->name, call.result, test->expected, setup, call.valid, peer.armed_frames,
            peer.selected, peer.flushes, peer.clean);
    }
    return passed;
}


/* EN: Run all stage, race and pre-cancellation scenarios; return nonzero on any failure.
 * RU: Проверить ожидания на этапах, гонки с ответом и предварительную отмену. */
int main(void)
{
    static const Scenario tests[] = {
        {"before-create", Create, 0, 0, 0, STYXRPC_IO_CANCELLED},
        {"path-walk", PathStat, Twalk, 1, 0, STYXRPC_IO_CANCELLED},
        {"fragmented-walk", FileOpen, Twalk, 2, 0, STYXRPC_IO_CANCELLED},
        {"path-stat", PathStat, Tstat, 1, 0, STYXRPC_IO_CANCELLED},
        {"held-stat", FileStat, Tstat, 1, 0, STYXRPC_IO_CANCELLED},
        {"open", FileOpen, Topen, 1, 0, STYXRPC_IO_CANCELLED},
        {"open-acknowledged", FileOpen, Topen, 1, 1, STYXRPC_IO_OK},
        {"directory-open", DirectoryOpen, Topen, 1, 0, STYXRPC_IO_CANCELLED},
        {"directory-read", DirectoryNext, Tread, 1, 0, STYXRPC_IO_CANCELLED},
        {"directory-acknowledged", DirectoryNext, Tread, 1, 1, STYXRPC_IO_OK},
        {"create-check", Create, Tstat, 1, 0, STYXRPC_IO_CANCELLED},
        {"create", Create, Tcreate, 1, 0, STYXRPC_IO_UNCERTAIN},
        {"create-acknowledged", Create, Tcreate, 1, 1, STYXRPC_IO_OK},
        {"mkdir", MakeDirectory, Tcreate, 1, 0, STYXRPC_IO_UNCERTAIN},
        {"remove-check", Remove, Twalk, 1, 0, STYXRPC_IO_CANCELLED},
        {"remove", Remove, Tremove, 1, 0, STYXRPC_IO_UNCERTAIN},
        {"remove-acknowledged", Remove, Tremove, 1, 1, STYXRPC_IO_OK},
        {"module-capture", ModuleCapture, Twalk, 1, 0, STYXRPC_IO_CANCELLED},
        {"module-exports", ModuleExports, Tread, 1, 0, STYXRPC_IO_CANCELLED},
        {"module-check", ModuleCheck, Tread, 1, 0, STYXRPC_IO_CANCELLED},
        {"scalar-admission", ScalarCall, Tread, 1, 0, STYXRPC_IO_CANCELLED}
    }; /* Exact real-codec cases, independent sessions. */
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        if (!exercise(&tests[i]))
        {
            return 1;
        }
    }
    puts("PASS original Styx VFS control cancellation");
    return 0;
}
