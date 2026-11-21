/* Copyright (c) 2026 bhgv
 * EN: Verify caller identity across reverse Styx program directory captures.
 * RU: Проверить идентичность клиента при захвате каталогов через обратный Styx.
 */

/* EN: Return the marker of the selected process directory; self is borrowed.
 * RU: Вернуть метку выбранного каталога процесса; self заимствован. */
static int program_method(void *state, DVM_Provider *self, int64_t argument, int64_t *value)
{
    WalkEntity *expected = state;
    DVM_StyxCallerKey key;
    check(dvm_provider_styx_caller(self, &key) && key.pid == (uint32_t)argument &&
        !memcmp(&key.session, expected, sizeof(*expected)), "captured caller key belongs to the mounted session");
    FVM_FileInfo info;
    ++calls;
    *value = dvm_provider_info(self, "first", &info) == FVM_OK ? 41 : 42;
    return *value == 41 || dvm_provider_info(self, "second", &info) == FVM_OK;
}


/* EN: Select two private process directories over reverse Styx and reject a missing PID.
 * RU: Выбрать два частных каталога через встречный Styx и отклонить отсутствующий PID. */
static void mounted_program(WalkEntity *session)
{
    int pair[2];
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
    Styxops readonly = {0};
    check(!socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "program carrier");
    int breaker = dup(pair[0]);
    check(breaker >= 0, "program carrier breaker");
    PhyBidir *caller = phybidircreate(phyposixfd(pair[0], caps), 1024);
    PhyBidir *provider = phybidircreate(phyposixfd(pair[1], caps), 1024);
    check(caller && provider, "program bidirectional mux");
    memset(&owner, 0, sizeof(owner));
    check(!styxphyinit(&owner, &readonly, phybidirchannel(caller, 1), 0555, 1, 2) &&
        styxadddir(&owner, Qroot, 1, ".prog", 0555, "caller") &&
        styxadddir(&owner, 1, 2, "41", 0555, "caller") &&
        styxadddir(&owner, 1, 3, "42", 0555, "caller") &&
        styxaddfile(&owner, 2, 4, "first", 0444, "caller") &&
        styxaddfile(&owner, 3, 5, "second", 0444, "caller"), "publish reverse program directories");
    pthread_t worker;
    check(!pthread_create(&worker, NULL, serve, NULL), "start reverse program server");
    StyxClient *reverse = styxclientmountphy(phybidirchannel(provider, 1), 256, "provider", "", 3000, 4);
    DVM_Provider *root = reverse ? styxmountproviderat(reverse, ".", FVM_ACCESS_READ) : NULL;
    check(root && dvm_provider_walk_get(root, session), "mount reverse program root");
    StyxScalarFunction function = {
        .request_tag = STYX_FUNCTION_CALLER_TAG, .reply_tag = STYX_FUNCTION_REPLY_TAG,
        .state = session, .program_root = root, .program_method = program_method
    };
    StyxUnibusServer *server = styxunibusserverstartfunction(phybidirchannel(provider, 0),
        "compute.dis", "compute", &function);
    check(server != NULL, "publish context-aware function");
    StyxClient *forward = styxclientmountphy(phybidirchannel(caller, 0), 256, "caller", "", 3000, 4);
    check(forward != NULL, "mount context-aware function");
    const uint32_t pids[] = {41, 42, 41, 43};
    for (size_t i = 0; i < sizeof(pids) / sizeof(pids[0]); i++)
    {
        uint32_t pid = pids[i];
        StyxFid *fid = styxclientwalk(forward, "mounted/compute.dis/compute");
        unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
        size_t size;
        StyxTransfer transfer;
        check(fid && styxclientopen(fid, ORDWR) &&
            styxscalarcallerwrite(pid, pid, bytes, sizeof(bytes), &size), "prepare scoped invocation");
        styxclientwrite(fid, 0, bytes, size, &transfer);
        check(transfer.success == (pid != 43), "reject missing caller directory before invocation");
        if (transfer.success)
        {
            StyxScalarKind kind;
            int64_t value;
            styxclientread(fid, 0, bytes, sizeof(bytes), &transfer);
            check(transfer.success && styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, transfer.count, &kind, &value) &&
                kind == STYX_SCALAR_VALUE && value == pid, "read result from selected .self");
        }
        styxclientclunk(fid);
    }
    shutdown(breaker, SHUT_RDWR);
    check(styxunibusserverjoin(server) && !pthread_join(worker, NULL), "join reverse and forward servers");
    styxclientfree(forward);
    dvm_provider_release(root);
    phybidirfree(caller);
    phybidirfree(provider);
    close(breaker);
}
