/* Copyright (c) 2026 bhgv
 * EN: Boot a local Dis program with a bidirectional Styx mount at /peer.
 * RU: Запуск локальной программы Dis с двусторонним маунтом Styx в /peer.
 */
#include "config.h"
#include "caller_styx.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


/* EN: Install the mount as a capability in a fresh hosted namespace and run the guest.
 * RU: Установить маунт в новом пространстве имён и запустить программу. */
static int run_mounted(int argc, char **argv, DVM_Provider *mount)
{
    char cwd[DVM_NS_PATH_LIMIT];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        return 1;
    }
    DVM_LaunchHandoff handoff = {.local_role = DVM_ORIGIN_TERMINAL};
    handoff.group = dvm_namespace_create(cwd, "/", NULL);
    strcpy(handoff.description.module, "styx-mounted");
    DVM_NamespacePlan target;
    int installed = handoff.group && dvm_namespace_pin(handoff.group) &&
        dvm_namespace_plan(handoff.group, "/peer", &target) &&
        dvm_namespace_mount_provider(handoff.group, &target, mount, "/styx-peer") == 1;
    int result = installed ? dvm_runtime_main_with_handoff(argc, argv, &handoff) : 1;
    if (!installed)
    {
        fprintf(stderr, "disvm: cannot mount Styx at /peer\n");
    }
    dvm_namespace_release(handoff.group);
    return result;
}


/* EN: Consume a delegated stream duplicate and run with forward channel 0 and reverse channel 1.
 * RU: Принять дубликат выданного потока и запустить с прямым каналом 0 и встречным 1. */
int dvm_styx_mount_cli(int argc, char **argv)
{
    if (argc < 5 || strcmp(argv[3], "--"))
    {
        fprintf(stderr, "usage: disvm --styx-mount-fd FD -- [VM OPTIONS] BOOTSTRAP.dis\n");
        return 2;
    }
    int fd = dvm_terminal_connected(argv[2]);
    int breaker = fd >= 0 ? dup(fd) : -1;
    if (breaker < 0)
    {
        if (fd >= 0)
        {
            close(fd);
        }
        fprintf(stderr, "disvm: expected a delegated connected stream FD >= 3\n");
        return 2;
    }
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    StyxCallerServer *reverse = mux ? styxcallerserver(phybidirchannel(mux, 1)) : NULL;
    StyxClient *client = reverse ? styxclientmountphy(phybidirchannel(mux, 0), 4096,
        "caller", "", 5000, 8) : NULL;
    DVM_Provider *mount = client ? styxcallermount(reverse, client, FVM_ACCESS_MASK) : NULL;
    int result = 1;
    if (mount)
    {
        result = run_mounted(argc - 3, argv + 3, mount);
    }
    else
    {
        fprintf(stderr, "disvm: cannot initialize bidirectional Styx mount\n");
    }
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxcallerjoin(reverse);
    dvm_provider_release(mount);
    phybidirfree(mux);
    return result;
}
