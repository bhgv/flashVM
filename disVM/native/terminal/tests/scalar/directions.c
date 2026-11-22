/* Copyright (c) 2026 bhgv
 * EN: Verify separate process publications in both Styx directions on one carrier.
 * RU: Проверка независимых процессов в двух направлениях Styx на одном соединении.
 */
#include "caller_styx.h"
#include "unibus_inbox.h"
#include "flashvm/srecord.h"
#include "export.h"
#include "mount.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>


/* EN: Check one invariant before advancing the isolated SDK fixture.
 * RU: Проверить одно условие до продолжения изолированного стенда SDK. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "Styx directions: %s\n", message);
        exit(1);
    }
}


/* EN: Create one immutable readable S-record in a native single-shot inbox.
 * RU: Создать одну неизменяемую запись в нативном одноразовом приёмнике. */
static DVM_Provider *file(char value)
{
    DVM_Provider *root = styxunibusinbox(1);
    DVM_VfsHandle writer = {0};
    unsigned char wire[16];
    size_t size;
    check(root && fvm_srecord_write(1, &value, 1, wire, sizeof(wire), &size) == FVM_S_OK &&
        dvm_provider_open(root, ".", "wo", &writer) == FVM_OK &&
        dvm_vfs_write_chunk(&writer, wire, size).status == FVM_OK, "build fixture file");
    dvm_vfs_close(&writer);
    return root;
}


/* EN: Create one owned publication with the chosen descriptor and variable bytes.
 * RU: Создать своё дерево с заданным содержимым дескриптора и переменной. */
static DVM_Provider *publication(char descriptor, char variable)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    DVM_Provider *fd = file(descriptor);
    DVM_Provider *var = file(variable);
    check(view && fd && var &&
        dvm_view_map(view, "/.prog/42/.fd/7", fd, ".", FVM_ACCESS_READ, 0) &&
        dvm_view_map(view, "/.prog/42/.var/cell", var, ".", FVM_ACCESS_READ, 0), "construct process tree");
    dvm_provider_release(fd);
    dvm_provider_release(var);
    DVM_Provider *root = dvm_view_freeze(view);
    check(root != NULL, "freeze process tree");
    return root;
}


/* EN: Read the first byte from the exported file on this particular mount.
 * RU: Прочитать первый байт выданного файла через данный маунт. */
static void expect(DVM_Provider *mount, const char *path, char expected)
{
    DVM_VfsHandle opened = {0};
    check(dvm_provider_open(mount, path, "r", &opened) == FVM_OK, path);
    unsigned char bytes[16];
    DVM_IoResult got = dvm_vfs_read_chunk(&opened, bytes, sizeof(bytes));
    FVM_SRecord record;
    size_t used;
    check(got.status == FVM_OK && fvm_srecord_read(bytes, got.count, sizeof(bytes), &record, &used) == FVM_S_OK &&
        used == got.count && record.tag == 1 && record.size == 1 &&
        record.payload[0] == (unsigned char)expected, "wrong directional value");
    dvm_vfs_close(&opened);
}


/* EN: Publish the same PID in both directions, revoke one and check the other.
 * RU: Опубликовать одинаковый PID в двух направлениях, отозвать одну выдачу. */
int main(void)
{
    const char *fd_path = ".prog/42/.fd/7";
    const char *var_path = ".prog/42/.var/cell";
    int sockets[2];
    check(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0, "create carrier");
    int breaker = dup(sockets[0]);
    check(breaker >= 0, "retain shutdown duplicate");
    unsigned flags = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    PhyBidir *a = phybidircreate(phyposixfd(sockets[0], flags), 65536);
    PhyBidir *b = phybidircreate(phyposixfd(sockets[1], flags), 65536);
    check(a && b, "multiplex carrier");
    StyxCallerServer *a_server = styxcallerserver(phybidirchannel(a, 1));
    StyxCallerServer *b_server = styxcallerserver(phybidirchannel(b, 0));
    check(a_server && b_server, "start independent Styx servers");
    StyxClient *a_client = styxclientmountphy(phybidirchannel(a, 0), 4096, "A", "", 3000, 4);
    StyxClient *b_client = styxclientmountphy(phybidirchannel(b, 1), 4096, "B", "", 3000, 4);
    check(a_client && b_client, "attach both directions");
    DVM_Provider *to_b = styxcallermount(a_server, a_client, FVM_ACCESS_MASK);
    DVM_Provider *to_a = styxcallermount(b_server, b_client, FVM_ACCESS_MASK);
    DVM_CallerBinding a_grants, b_grants;
    check(to_a && to_b && dvm_provider_caller_binding(to_b, &a_grants) &&
        dvm_provider_caller_binding(to_a, &b_grants), "get bound direction controls");
    DVM_Provider *a_tree = publication('A', 'a');
    DVM_Provider *b_tree = publication('B', 'b');
    void *a_lease = a_grants.begin(a_grants.state, to_b, a_tree, 42);
    void *b_lease = b_grants.begin(b_grants.state, to_a, b_tree, 42);
    check(a_lease && b_lease, "publish identical numeric PIDs independently");

    expect(to_b, fd_path, 'B');
    expect(to_a, fd_path, 'A');
    expect(to_b, var_path, 'b');
    expect(to_a, var_path, 'a');
    DVM_VfsHandle retired = {0};
    check(dvm_provider_open(to_b, fd_path, "r", &retired) == FVM_OK, "pin B's old fid");
    b_grants.end(b_grants.state, b_lease);
    DVM_Provider *replacement = publication('C', 'c');
    void *next = b_grants.begin(b_grants.state, to_a, replacement, 42);
    check(next != NULL, "replace B's offer with the same PID");
    char byte;
    check(dvm_vfs_read_chunk(&retired, &byte, 1).status != FVM_OK, "retired fid inherited the replacement");
    expect(to_b, fd_path, 'C');
    expect(to_b, var_path, 'c');
    expect(to_a, fd_path, 'A');
    expect(to_a, var_path, 'a');

    dvm_vfs_close(&retired);
    b_grants.end(b_grants.state, next);
    a_grants.end(a_grants.state, a_lease);
    dvm_provider_release(replacement);
    dvm_provider_release(b_tree);
    dvm_provider_release(a_tree);
    shutdown(breaker, SHUT_RDWR);
    close(breaker);
    styxcallerjoin(a_server);
    styxcallerjoin(b_server);
    dvm_provider_release(to_a);
    dvm_provider_release(to_b);
    phybidirfree(a);
    phybidirfree(b);
    puts("PASS separate Styx directional process grants");
    return 0;
}
