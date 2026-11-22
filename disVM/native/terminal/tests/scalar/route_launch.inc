/* Copyright (c) 2026 bhgv
 * EN: Exercise independent launches beside mounted functions on a single session.
 * RU: Проверить независимые запуски рядом с вызовами функций на одной сессии.
 */

static void launch_write(StyxFid *fid, uint32_t pid, int expected)
{
    DVM_LaunchProgram program = {.pid = pid};
    unsigned char header[STYX_LAUNCH_PROGRAM_HEADER], wire[64];
    size_t size;
    check(styxlaunchprogramencode(header, &program) &&
        fvm_srecord_write(REQUEST_TAG, header, sizeof(header), wire, sizeof(wire), &size) == FVM_S_OK,
        "format one scoped launch");
    StyxTransfer result;
    styxclientwrite(fid, 0, wire, size, &result);
    if (!!result.success != expected || (expected && result.count != size))
    {
        fprintf(stderr, "launch PID %u: success=%d count=%zu expected=%d\n",
            pid, result.success, result.count, expected);
        check(0, "admit a complete launch once per fid");
    }
}

static StyxLaunchTicket *launch_take(StyxUnibusServer *server, uint32_t pid)
{
    unsigned char wire[64];
    size_t size, used;
    FVM_SRecord record;
    DVM_LaunchProgram program;
    StyxLaunchTicket *ticket = styxunibusservernextlaunch(server, wire, sizeof(wire), &size);
    check(ticket && fvm_srecord_read(wire, size, sizeof(wire), &record, &used) == FVM_S_OK &&
        used == size && styxlaunchprogramdecode(record.payload, record.size, &program) &&
        program.pid == pid, "dequeue the expected launch");
    return ticket;
}

static void launch_status(StyxFid *fid, const char *expected)
{
    char bytes[8];
    size_t size = expected ? strlen(expected) : 0;
    StyxTransfer result;
    styxclientread(fid, 0, bytes, sizeof(bytes), &result);
    check(result.success == (expected != NULL) && (!expected ||
        (result.count == size && !memcmp(bytes, expected, size))), "launch status belongs to its fid");
}

static int cancelled_launch;

static void cancel_child(int pid, void *ticket)
{
    check(ticket != NULL && pid > 0, "cancel only a registered launch");
    __atomic_store_n(&cancelled_launch, pid, __ATOMIC_RELEASE);
}

static void check_launch_clunk(StyxUnibusServer *server, StyxClient *client, int clunk_first)
{
    int pid = clunk_first ? 81 : 82;
    StyxFid *fid = styxclientwalk(client, ".unibus");
    check(fid && styxclientopen(fid, ORDWR), "open a cancellable launch");
    launch_write(fid, (uint32_t)pid, 1);
    StyxLaunchTicket *ticket = launch_take(server, (uint32_t)pid);
    if (clunk_first)
    {
        styxclientclunk(fid);
        styxunibuslaunchstarted(ticket, pid, cancel_child);
    }
    else
    {
        styxunibuslaunchstarted(ticket, pid, cancel_child);
        styxclientclunk(fid);
    }
    check(__atomic_load_n(&cancelled_launch, __ATOMIC_ACQUIRE) == pid,
        "clunk cancels a dequeued launch before or after registration");
    styxunibuslaunchcomplete(ticket, 1);
}

static void check_shared_launch(StyxUnibusServer *server, StyxClient *client)
{
    StyxFid *first = styxclientwalk(client, ".unibus");
    StyxFid *second = styxclientwalk(client, ".unibus");
    StyxFid *discarded = styxclientwalk(client, ".unibus");
    StyxFid *legacy = styxclientwalk(client, ".unibus");
    StyxFid *exit_status = styxclientwalk(client, ".exit");
    StyxFid *image = styxclientwalk(client, "implementation.dis");
    StyxFid *declaration = styxclientwalk(client, ".interface");
    check(first && second && discarded && legacy && exit_status && image && declaration &&
        styxclientopen(first, ORDWR) && styxclientopen(second, ORDWR) && styxclientopen(discarded, ORDWR) &&
        styxclientopen(legacy, OWRITE) && styxclientopen(exit_status, OREAD) &&
        styxclientopen(image, OREAD) && styxclientopen(declaration, OREAD), "open launch nodes beside library");
    styxclientclunk(image);
    styxclientclunk(declaration);

    launch_write(first, 73, 1);
    launch_write(discarded, 76, 1);
    styxclientclunk(discarded);
    launch_write(second, 77, 1);
    launch_write(legacy, 78, 1);
    styxclientclunk(legacy);
    StyxLaunchTicket *a = launch_take(server, 73);
    StyxLaunchTicket *b = launch_take(server, 77);
    StyxLaunchTicket *old = launch_take(server, 78);
    launch_status(first, "");
    styxunibuslaunchcomplete(b, 1);
    launch_status(second, "error\n");
    launch_status(first, "");
    launch_status(exit_status, "");
    styxunibuslaunchcomplete(a, 0);
    styxunibuslaunchcomplete(old, 0);
    launch_status(first, "ok\n");
    launch_status(second, "error\n");
    launch_status(exit_status, "ok\n");
    launch_write(first, 75, 0);
    StyxFid *unsubmitted = styxclientwalk(client, ".unibus");
    check(unsubmitted && styxclientopen(unsubmitted, ORDWR), "open an unsubmitted launch fid");
    launch_status(unsubmitted, NULL);
    styxclientclunk(unsubmitted);
    styxclientclunk(first);
    styxclientclunk(second);
    styxclientclunk(exit_status);
    check_launch_clunk(server, client, 0);
    check_launch_clunk(server, client, 1);
}
