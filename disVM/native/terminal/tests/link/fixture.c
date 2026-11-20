/* Copyright (c) 2026 bhgv
 * EN: Original Styx server and explicit bidir pumps on an isolated native carrier for terminal-loss tests.
 * RU: Original Styx server и явные bidir pumps на изолированном native carrier для terminal-loss tests.
 */
#include <sys/socket.h>
#include <unistd.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "client.h"
#include "server.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#ifdef ORDERED_GUEST_FILE_TEST
#include "file.h"
#include "file_provider.h"
#include "held_export.h"
#include "launch_publication.h"
#include "../../../launch/request.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#endif
static Styxserver server;    /* Original native server, owned through join. */
#ifdef ORDERED_GUEST_FILE_TEST
static pid_t worker;         /* Independent server process, reaped after client close. */
#else
static pthread_t worker;     /* Dedicated original server owner. */
#endif
static PhyBidir *local;      /* Host-side explicit bidir pump. */
static PhyBidir *remote;     /* Server-side explicit bidir pump. */
static int breaker = -1;    /* Owned duplicate used only to break the real physical carrier. */


/* EN/RU: Original fragmented S-record fixture callbacks, called solely by original server owner.
 * Исходные fragmented S-record callbacks, только на original server owner.
 */
extern char *terminalread(Styxserver *, char *, ulong *, vlong);


/* EN/RU: Original precise-prefix S-record command writer; borrowed buffers on server owner.
 * Исходный precise-prefix writer S-команд, buffers borrowed на server owner.
 */
extern char *terminalwrite(char *, ulong *, vlong);

#ifdef ORDERED_GUEST_FILE_TEST


/* EN/RU: Own actual compiled readonly asset storage through original server join.
 * Владеть настоящим compiled readonly asset-storage до join original server.
 */
extern void fixtureimagesinit(Styxserver *);


/* EN/RU: Serve one original remote offset/count, borrowed buffers, short prefixes only.
 * Обслужить исходные remote offset/count, buffers borrowed, только short prefixes.
 */
extern char *fixtureimagesread(Qid, char *, ulong *, vlong);


/* EN/RU: Release image storage after every original server callback has retired.
 * Освободить image-storage после завершения всех original server callbacks.
 */
extern void fixtureimagesfree(void);
static StyxLaunchPublication owner_process;
static StyxBacking *owner_result; /* Published exit-status stream. */
static char *owner_argv[4]; /* Borrowed environment and fixed test command. */
extern char *fixturelaunchwrite(char *, ulong *, vlong); /* Exact launch request writer. */

static const unsigned long held_input = 12;  /* Qid of the transferred read file. */
static const unsigned long held_output = 13; /* Qid of the transferred write file. */
static const unsigned long numbered_root = 14; /* Directory qid for .fd. */
static const unsigned long numbered_first = 15; /* Qid base for numbered fixture files. */
static StyxBacking *held[2];                  /* Named backings until server shutdown. */
static StyxBacking *numbered[4];              /* Numbered backings until server shutdown. */


/* EN/RU: Publish a native open at offset five, unlinking the source of the read grant.
 * Опубликовать открытый файл с позиции пять, удалив исходное имя файла для чтения. */
static StyxBacking *add_held(unsigned long parent, unsigned long qid, char *node, const char *path, unsigned access)
{
    FVM_PortFile *file = NULL;
    DVM_VfsHandle source = {0};
    int reading = access == FVM_ACCESS_READ;
    long position;
    const char *mode = reading ? "r" : access == FVM_ACCESS_WRITE ? "wo" : "rwo";
    if (fvm_port_file_open(path, mode, &file) != FVM_OK ||
        !dvm_vfs_adopt(&source, file, NULL) ||
        dvm_vfs_seek(&source, 5, FVM_FILE_SEEK_START, &position) != FVM_OK ||
        position != 5 || (reading && unlink(path)))
    {
        exit(3);
    }
    DVM_Provider *grant = styxheldexport(&source, access);
    StyxBacking *backing = styxbacking(grant);
    dvm_provider_release(grant);
    StyxMeta info;
    if (!backing || !styxbackinginfo(backing, &info))
    {
        exit(3);
    }
    int permissions = ((access & FVM_ACCESS_READ) ? 0444 : 0) |
                      ((access & FVM_ACCESS_WRITE) ? 0222 : 0);
    Styxfile *endpoint = styxaddfile(&server, parent, qid, node, permissions, "fixture");
    if (!endpoint)
    {
        exit(3);
    }
    endpoint->d.length = info.length;
    return backing;
}


/* EN/RU: Map the test server's exact qids to retained single-file backings.
 * Сопоставить точные qid тестового сервера с удерживаемыми файлами. */
static StyxBacking *held_backing(uvlong qid)
{
    if (qid == 21)
    {
        return owner_result;
    }
    if (qid == held_input || qid == held_output)
    {
        return held[qid == held_output];
    }
    return qid >= numbered_first && qid < numbered_first + nelem(numbered) ?
        numbered[qid - numbered_first] : NULL;
}


/* EN/RU: Transfer the one permitted open to the current Styx fid.
 * Передать единственное допустимое открытие текущему Styx fid. */
static char *open_held(Qid *qid, int mode)
{
    StyxBacking *backing = held_backing(qid->path);
    if (!backing)
    {
        return nil;
    }
    StyxOpen *opened = styxfileopen(backing, mode);
    if (!opened || styxsetaux(&server, opened) < 0)
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}
#endif


#ifdef ORDERED_GUEST_FILE_TEST
/* EN/RU: Configure the owner's fixed command before exposing its request file.
 * Настроить фиксированную команду владельца до публикации узла запуска. */
static int prepare_owner(void)
{
    owner_argv[0] = getenv("DVM_TEST_OWNER_RUNTIME");
    owner_argv[1] = getenv("DVM_TEST_OWNER_MODE");
    owner_argv[2] = "owner.dis";
    if (!owner_argv[1] || !styxlaunchpublicationinit(&owner_process, 17, "owner", owner_argv) ||
        !styxlaunchprocesspayload(&owner_process.process, 3))
    {
        return 0;
    }
    DVM_Provider *file = styxlaunchpublicationfile(&owner_process, 1);
    owner_result = styxbacking(file);
    dvm_provider_release(file);
    return owner_result != NULL;
}
#endif


/* EN/RU: Serve only the granted records node; original core owns walk, permissions and fid lifecycle.
 * Обслужить records node; walk, права и fid lifecycle принадлежат original core.
 */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
#ifdef ORDERED_GUEST_FILE_TEST
    if (held_backing(qid.path))
    {
        StyxTransfer result;
        if (offset < 0)
        {
            return Ebadarg;
        }
        styxfileread(styxgetaux(&server), offset, bytes, *count, &result);
        if (!result.success || !result.confirmed)
        {
            return Eperm;
        }
        *count = result.count;
        return nil;
    }
    if (qid.path != 7)
    {
        return fixtureimagesread(qid, bytes, count, offset);
    }
#endif
    return qid.path == 7 ? terminalread(&server, bytes, count, offset) : Ebadarg;
}


/* EN/RU: Write only records; buffers/count borrowed from original core through return.
 * Записать только records; buffers/count borrowed из original core до return.
 */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
#ifdef ORDERED_GUEST_FILE_TEST
    if (held_backing(qid.path))
    {
        StyxTransfer result;
        if (offset < 0)
        {
            return Ebadarg;
        }
        styxfilewrite(styxgetaux(&server), offset, bytes, *count, &result);
        if (!result.success || !result.confirmed || result.uncertain)
        {
            return Eperm;
        }
        *count = result.count;
        return nil;
    }
    if (qid.path == 7 && offset >= 0 && owner_result)
    {
        int decision = styxlaunchpublicationdispatch(&owner_process, offset, bytes, *count);
        if (decision)
        {
            return decision > 0 ? nil : Ebadarg;
        }
    }
    if (qid.path == 11)
    {
        return fixturelaunchwrite(bytes, count, offset);
    }
#endif
    return qid.path == 7 ? terminalwrite(bytes, count, offset) : Ebadarg;
}
static Styxops operations = { /* Server borrows static operations until join. */
    .read = readfile, .write = writefile,
#ifdef ORDERED_GUEST_FILE_TEST
    .open = open_held, .freefid = styxfileclose,
#endif
};


/* EN/RU: Own original server callbacks until physical/logical teardown, consume server PHY once.
 * Владеть original server callbacks до teardown, поглотить server PHY один раз.
 */
static void *serve(void *unused)
{
    USED(unused);
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return nil;
}


/* EN/RU: Consume fd and start the original server on the explicit logical stream.
 * Поглотить fd и настроить исходный server на явном logical stream. */
static void server_start(int fd, unsigned caps)
{
    const char *records = "records";
#ifdef ORDERED_GUEST_FILE_TEST
    records = STYX_UNIBUS_NODE;
    if (getenv("DVM_TEST_TERMINAL_CUSTOM"))
    {
        records = "job-records";
    }
#endif
    remote = phybidircreate(phyposixfd(fd, caps), 4096);
    if (!remote || styxphyinit(&server, &operations, phybidirchannel(remote, 0), 0555, 1, 2) ||
        !styxaddfile(&server, Qroot, 7, (char *)records, 0666, "fixture"))
    {
        exit(3);
    }
}


#ifdef ORDERED_GUEST_FILE_TEST
/* EN/RU: Build a private TCP connection on loopback; return both owned descriptors.
 * Создать частное TCP-соединение через loopback, вернуть два собственных fd. */
static void loopback_pair(int pair[2])
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    socklen_t size = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (listener < 0 || bind(listener, (struct sockaddr *)&address, size) ||
        getsockname(listener, (struct sockaddr *)&address, &size) || listen(listener, 1))
    {
        exit(3);
    }
    pair[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (pair[0] < 0 || connect(pair[0], (struct sockaddr *)&address, size))
    {
        exit(3);
    }
    pair[1] = accept(listener, NULL, NULL);
    int immediate = 1; /* The fixture deliberately splits reads and write acknowledgements. */
    if (pair[1] < 0 || close(listener) ||
        setsockopt(pair[0], IPPROTO_TCP, TCP_NODELAY, &immediate, sizeof(immediate)) ||
        setsockopt(pair[1], IPPROTO_TCP, TCP_NODELAY, &immediate, sizeof(immediate)))
    {
        exit(3);
    }
}
#endif


#ifdef ORDERED_GUEST_FILE_TEST
/* EN/RU: Start the real loopback server and give the launcher an owned carrier FD.
 * Запустить настоящий loopback server и передать лаунчеру собственный fd. */
int linkfixturefd(void)
{
    int pair[2];
    loopback_pair(pair);
    worker = fork();
    if (worker < 0)
    {
        exit(3);
    }
    if (!worker)
    {
        close(pair[0]);
        signal(SIGPIPE, SIG_IGN);
        if (fcntl(pair[1], F_SETFD, FD_CLOEXEC) < 0)
        {
            _exit(3);
        }
        server_start(pair[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE);
        if (getenv("DVM_TEST_TERMINAL_SUBMIT") && !prepare_owner())
        {
            _exit(3);
        }
        fixtureimagesinit(&server);
        held[0] = add_held(Qroot, held_input, "held-input", "held-source.bin", FVM_ACCESS_READ);
        held[1] = add_held(Qroot, held_output, "held-output", "held-target.bin", FVM_ACCESS_WRITE);
        if (getenv("DVM_TEST_TERMINAL_NUMBERED"))
        {
            char *directory = getenv("DVM_TEST_TERMINAL_SUBMIT") ? "owner-fds" : ".fd";
            if (!styxadddir(&server, Qroot, numbered_root, directory, 0555, "fixture"))
            {
                _exit(3);
            }
            numbered[0] = add_held(numbered_root, numbered_first, "0", "numbered-in.bin", FVM_ACCESS_READ);
            numbered[1] = add_held(numbered_root, numbered_first + 1, "1", "numbered-out.bin", FVM_ACCESS_WRITE);
            numbered[2] = add_held(numbered_root, numbered_first + 2, "2", "numbered-err.bin", FVM_ACCESS_WRITE);
            numbered[3] = add_held(numbered_root, numbered_first + 3, "3", "numbered-rw.bin", FVM_ACCESS_READ | FVM_ACCESS_WRITE);
        }
        serve(NULL);
        styxbackingfree(held[0]);
        styxbackingfree(held[1]);
        for (size_t i = 0; i < nelem(numbered); i++)
        {
            styxbackingfree(numbered[i]);
        }
        fixtureimagesfree();
        phybidirfree(remote);
        if (getenv("DVM_TEST_TERMINAL_SUBMIT"))
        {
            int status;
            styxbackingfree(owner_result);
            if (styxlaunchpublicationfinish(&owner_process, &status) != 1 || status != 0)
            {
                _exit(3);
            }
        }
        _exit(0);
    }
    close(pair[1]);
    return pair[0];
}
#endif


/* EN/RU: Own real client/NULL on one logical stream of an explicitly bidir carrier.
 * Свой real client/NULL на logical stream явного bidir carrier. */
StyxClient *linkfixture(void)
{
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
#ifdef ORDERED_GUEST_FILE_TEST
    int fd = linkfixturefd();
#else
    int pair[2]; /* Connected carrier; each PHY consumes one descriptor. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair))
    {
        exit(3);
    }
    server_start(pair[1], caps);
    if (pthread_create(&worker, nil, serve, nil))
    {
        exit(3);
    }
    int fd = pair[0];
#endif
    breaker = dup(fd);
    local = phybidircreate(phyposixfd(fd, caps), 4096);
    if (breaker < 0 || !local)
    {
        exit(3);
    }
    return styxclientmountphy(phybidirchannel(local, 0), 256, "fixture", "", 3000, 4);
}


/* EN/RU: Break the actual carrier independently of control/client API; no fake client terminal notification.
 * Разорвать настоящий carrier независимо от control/client API, без fake terminal notification.
 */
void linkfixturebreak(void)
{
    if (shutdown(breaker, SHUT_RDWR))
    {
        exit(3);
    }
}


/* EN/RU: After all client/control users retire, join server and consume physical owners/descriptors.
 * После завершения client/control users сделать join server и поглотить physical owners/fds.
 */
void linkfixturefree(void)
{
#ifdef ORDERED_GUEST_FILE_TEST
    if (breaker >= 0)
    {
        shutdown(breaker, SHUT_RDWR); /* Wake the remote process before joining it. */
        phybidirfree(local);
        close(breaker);
    }
    int status;
    if (waitpid(worker, &status, 0) != worker || !WIFEXITED(status) || WEXITSTATUS(status))
    {
        exit(3);
    }
#else
    if (pthread_join(worker, nil))
    {
        exit(3);
    }
    phybidirfree(local);
    phybidirfree(remote);
    close(breaker);
#endif
}
