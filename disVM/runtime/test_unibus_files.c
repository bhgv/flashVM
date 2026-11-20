/* Copyright (c) 2026 bhgv
 * EN: Real pre-init ordered launch handoff, ordinary VFS record files and dedicated guest execution.
 * RU: Настоящий ordered launch handoff до init, обычные VFS record-файлы и dedicated guest execution.
 */
#include <sys/wait.h>
#include <unistd.h>
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runqueue.h"
#include "vfs/launch/runtime.h"
#include "vfs/export.h"
#include "../native/terminal/order/files/launch.h"
#include "../native/terminal/route/launch.h"
#include "../native/terminal/launch_send.h"
#include "../native/vfs/mount.h"
#include "../native/tests/launch/backend.h"
static LaunchState terminal; /* Unused local fixture root, disposed before remote preparation. */
static LaunchState execution; /* Temporary local fixture retires before the actual Styx code root is prepared. */
static StyxClient *connection; /* Original native session, independent of broker owners. */
static StyxOrderedFiles *files; /* Explicit lifecycle owner after namespace/FD disposal. */


/* EN/RU: Original fixture client, owned until explicit native cleanup, no guest or simulated protocol.
 * Исходный fixture client, owned до явной native-очистки, без guest/имитации протокола.
 */
extern StyxClient *linkfixture(void);
extern DVM_Provider *fixturereverseinbox(void); /* Borrowed caller reply inbox. */
extern int linkfixturefd(void); /* Owned fixture carrier for a separate CLI process. */


/* EN/RU: Join actual original server and physical owners after all client refs retire.
 * Завершить настоящий исходный server и physical owners после client refs.
 */
extern void linkfixturefree(void);


/* EN/RU: Resolve the exact provisioned request file carried by this advertisement.
 * Получить точный выданный файл запроса, связанный с объявлением. */
static DVM_LaunchAddress *wire_address(void *request)
{
    return styxterminallaunchaddress(request, 17);
}


/* EN/RU: Attach the test process identity to the selected module's launch request.
 * Добавить идентификатор тестового процесса к запросу запуска выбранного модуля. */
static DVM_LaunchAddress *wire_program_address(void *request)
{
    const DVM_LaunchProgram program = {42, 41, UINT32_C(1) << 2};
    return styxterminallaunchprogramaddress(request, 17, &program);
}


/* EN/RU: Bind the launch route to the VM's current caller instead of a stored PID.
 * Связать запуск с текущим вызывающим VM без фиксированного PID. */
static DVM_LaunchAddress *wire_caller_address(void *request)
{
    return styxterminallaunchcalleraddress(request, 17);
}


/* EN/RU: Release the route's owned request provider.
 * Освободить принадлежащий маршруту провайдер запросов. */
static void release_request(void *request)
{
    dvm_provider_release(request);
}


/* EN/RU: Add one executable alias with a retained Styx submit route.
 * Добавить исполняемый alias с удерживаемым маршрутом отправки Styx. */
static void map_routed(StyxClient *client, DVM_View *code, const char *endpoint,
    const char *image, const char *alias)
{
    launch_check(styxclientretain(client), "retain guest submit client");
    DVM_Provider *request = styxmountproviderat(client, endpoint, FVM_ACCESS_WRITE);
    launch_check(request && styxclientretain(client), "guest submit node");
    DVM_Provider *mounted = styxmountproviderlaunch(client, ".", FVM_ACCESS_READ | FVM_ACCESS_EXEC,
        image, !strcmp(alias, "/payload.dis") && getenv("DVM_TEST_TERMINAL_SCOPED") ?
            wire_caller_address : wire_address, request, release_request);
    launch_check(mounted && dvm_view_map(code, alias, mounted, image,
        FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0), "map guest launch alias");
    dvm_provider_release(mounted);
}


/* EN/RU: Select the bound remote Dis path and send to its owner over ordinary Styx.
 * Выбрать забинженный Dis и передать запрос владельцу через обычный Styx. */
static void submit_mounted(StyxClient *client, const DVM_LaunchOptions *options)
{
    launch_check(styxclientretain(client), "retain remote submit connection");
    DVM_Provider *request = styxmountproviderat(client, "submit", FVM_ACCESS_WRITE);
    launch_check(request && styxclientretain(client), "exact remote submit node");
    DVM_Provider *mounted = styxmountproviderlaunch(client, ".", FVM_ACCESS_READ | FVM_ACCESS_EXEC,
        "app.dis", getenv("DVM_TEST_TERMINAL_SCOPED") ? wire_program_address : wire_address,
        request, NULL);
    DVM_View *aliases = dvm_view_create(NULL, NULL, NULL);
    launch_check(mounted && aliases && dvm_view_map(aliases, "/bound.dis", mounted, "app.dis",
        FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0), "bind advertised Dis file");
    DVM_LaunchOptions visible = *options;
    visible.terminal = dvm_view_freeze(aliases);
    DVM_Namespace *group = dvm_launch_prepare(&visible);
    DVM_LaunchAddress *address = group ? dvm_launch_route_resolve(group, "/bound.dis", DVM_ORIGIN_TERMINAL) : NULL;
    dvm_namespace_release(group);
    dvm_provider_release(visible.terminal);
    dvm_provider_release(mounted);
    dvm_provider_release(request);
    const unsigned char arguments[] = {'r', 0, 'n'};
    launch_check(address && dvm_launch_address_send(address, arguments, sizeof(arguments)) == DVM_LAUNCH_DELIVERED,
        "remote owner acknowledged one launch request");
    dvm_launch_address_free(address);
}


/* EN/RU: Prepare actual remote-root ticket, take once, construct ordered broker, install file grants and invoke real startup.
 * Подготовить настоящий remote-root ticket, одноразовый take, ordered broker и file grants, вызвать настоящий startup.
 */
static DVM_LaunchHandoff *prepare_launch(void)
{
    launch_check(setenv("DVM_TEST_TERMINAL_SUBMIT", "1", 1) == 0, "enable routed submit fixture");
    connection = linkfixture();
    launch_check(connection != NULL, "real original server/client fixture");
    DVM_LaunchOptions options = launch_options(&terminal, &execution); /* Real private live-root preparation. */
    dvm_provider_release(options.terminal);
    options.terminal = NULL;
    dvm_provider_release(options.provider); /* Execution image root below uses actual original-Styx files instead. */
    launch_check(styxclientretain(connection), "retained execution asset session");
    DVM_Provider *assets = styxmountproviderat(connection, ".", FVM_ACCESS_MASK); /* Actual remote qid-backed images. */
    DVM_View *code = dvm_view_create(NULL, NULL, NULL); /* Do not publish the raw records node through the execution root either. */
    launch_check(assets && code && dvm_view_map(code, "/app.dis", assets, "app.dis", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0) &&
        dvm_view_map(code, "/dep.dis", assets, "dep.dis", FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0), "filtered actual Styx code grants");
    int scoped = getenv("DVM_TEST_TERMINAL_SCOPED") != NULL;
    if (getenv("DVM_TEST_TERMINAL_NUMBERED") && !scoped)
    {
        launch_check(dvm_view_map(code, "/.fd", assets, "owner-fds", FVM_ACCESS_MASK, DVM_VIEW_TREE),
            "publish the owner's descriptor directory in its root");
    }
    launch_check(dvm_view_map(code, "/runexample.dis", assets, "runexample.dis",
        FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0) && dvm_view_map(code, "/vfsctl.dis", assets, "vfsctl.dis",
        FVM_ACCESS_READ | FVM_ACCESS_EXEC, 0), "map runnable VFS examples");
    map_routed(connection, code, STYX_UNIBUS_NODE, "owner.dis", "/routed.dis");
    map_routed(connection, code, "submit", "app.dis", "/payload.dis");
    launch_check(dvm_view_map(code, "/run-result", assets, "execute-result", FVM_ACCESS_READ, 0),
        "map owner execution result");
    DVM_Provider *inbox = fixturereverseinbox();
    launch_check(inbox && dvm_view_map(code, "/reverse-exit", inbox, ".", FVM_ACCESS_READ, 0),
        "map read-only caller completion inbox");
    options.provider = dvm_view_freeze(code);
    submit_mounted(connection, &options);
    DVM_TerminalChannels ingress = {0};
    ingress.provider = assets;
    strcpy(ingress.receive, "launch");
    strcpy(ingress.send, STYX_UNIBUS_NODE);
    const StyxRouteType launch_type = {"disvm.launch", 1}; /* Dedicated fixture schema. */
    StyxBidirRoute *route = styxbidirbindingroutes(connection, &ingress, 128, 2,
        NULL, 0, &launch_type, 1);
    launch_check(route != NULL, "live launch dispatcher");
    DVM_LaunchSpec spec = {0}; /* Captured terminal channels and execution source are independently authorized. */
    spec.program_root = scoped ? dvm_provider_capture(assets, "loaded.dis") : NULL;
    launch_check(!scoped || spec.program_root, "grant the loaded module root");
    spec.policy = options.policy;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "provider/unibus-files.dis");
    if (scoped)
    {
        const uint32_t denied_pids[] = {99, 43};
        for (size_t i = 0; i < sizeof(denied_pids) / sizeof(denied_pids[0]); i++)
        {
            DVM_LaunchSpec invalid_scope = spec;
            invalid_scope.terminal = assets;
            invalid_scope.program.pid = denied_pids[i];
            invalid_scope.program.parent_pid = 41;
            DVM_LaunchRequest *missing = NULL;
            DVM_LaunchTicket *refusal = dvm_launch_request_create(&invalid_scope, &missing);
            launch_check(refusal && missing &&
                !dvm_launch_request_prepare(missing, options.provider, &options.provider_origin) &&
                dvm_launch_ticket_status(refusal) == DVM_LAUNCH_DENIED && !dvm_launch_request_take(missing),
                "missing process or malformed namespace is refused");
            dvm_launch_request_free(missing);
            dvm_launch_ticket_free(refusal);
        }
    }
    StyxTerminalPaths paths = {STYX_UNIBUS_NODE, STYX_UNIBUS_NODE, NULL}; /* Existing granted original-Styx endpoints. */
    DVM_LaunchService *service = dvm_launch_service_create(options.provider_origin.system, 1);
    launch_check(service != NULL, "provider launch service");
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(service);
    launch_check(endpoint != NULL, "submit-only terminal endpoint");
    unsigned char startup[32]; /* Private copy made by the live typed mailbox. */
    size_t startup_size = 0;
    DVM_LaunchTicket *ticket = NULL;
    launch_check(styxbidirroutetypedwait(route, 0, 5000), "live launch frame received");
    launch_check(styxbidirroutelaunchnext(route, 0, &spec, FVM_ACCESS_MASK, &paths, endpoint, 17,
        startup, sizeof(startup), &startup_size, &ticket) == STYX_CONTROL_RECORD &&
        startup_size == (scoped ? 16U : 3U), "dispatcher delivered complete launch request");
    dvm_provider_release(spec.program_root);
    styxbidirroutefree(route); /* The independent application record channel opens next. */
    launch_check(ticket && dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING, "queued terminal request");
    DVM_LaunchReview *review = dvm_launch_service_next(service);
    launch_check(review != NULL, "provider review");
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    memset(startup, 0, startup_size); /* Provider must see the copied original, including embedded NUL. */
    unsigned char copied[3];
    size_t extent = 0;
    launch_check(dvm_launch_arguments(request, 0, copied, sizeof(copied), &extent) &&
        extent == sizeof(copied) && copied[0] == 'r' && copied[1] == 0 && copied[2] == 'n',
        "provider received independent startup bytes");
    launch_check(ticket && request && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "actual remote request prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request); /* Owned actual one-use environment. */
    if (scoped)
    {
        launch_check(handoff && handoff->program && handoff->program_parent && handoff->program_vfs &&
            handoff->program_hidden_fds == (UINT32_C(1) << 2),
            "wire request pinned child and parent beneath the loaded module");
    }
    dvm_provider_release(assets);
    unsigned long tags[] = {0, 7, 9, 17}; /* Authorized application inboxes only. */
    StyxOrderedOptions ordered = {sizeof(ordered), 128, 16, 71, tags, sizeof(tags) / sizeof(tags[0])}; /* Same profile as native API acceptance. */
    const StyxLaunchFile invalid = {"/forbidden", 99, FVM_ACCESS_READ};
    DVM_Namespace *original = handoff->group;
    launch_check(!styxorderedlaunchfiles(connection, handoff, &ordered, &invalid, 1) &&
        handoff->group == original, "unauthorized tag preserves original handoff");
    const StyxLaunchFile mappings[] = {
        {"/seven", 7, FVM_ACCESS_READ | FVM_ACCESS_WRITE},
        {"/nine", 9, FVM_ACCESS_READ},
        {"/launch-exit", 17, FVM_ACCESS_READ},
        {"/zero", 0, FVM_ACCESS_READ | FVM_ACCESS_WRITE}
    };
    const StyxLaunchFd absent = {"/input", "missing-file", FVM_ACCESS_READ};
    size_t count = sizeof(mappings) / sizeof(mappings[0]);
    launch_check(!styxorderedlaunchfileswithfds(connection, handoff, &ordered, mappings, count, &absent, 1) &&
        handoff->group == original, "missing FD grant preserves original handoff and broker");
    const StyxLaunchFd held[] = {
        {"/input", "held-input", FVM_ACCESS_READ},
        {"/output", "held-output", FVM_ACCESS_WRITE}
    };
    files = styxorderedlaunchfileswithfds(connection, handoff, &ordered, mappings, count,
        held, sizeof(held) / sizeof(held[0]));
    launch_check(files && handoff->group, "filtered file set and private launch namespace");
    dvm_launch_ticket_free(ticket);
    dvm_launch_review_free(review);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(service);
    dvm_provider_release(options.provider);
    return handoff;
}


/* EN/RU: Delegate a real stream to the actual configured CLI and reap both processes.
 * Передать реальный stream штатному CLI и дождаться обоих процессов. */
static int run_cli(const char *runtime, const char *mode, const char *bootstrap)
{
    int fd = linkfixturefd();
    char descriptor[24];
    snprintf(descriptor, sizeof(descriptor), "%d", fd);
    pid_t child = fork();
    launch_check(child >= 0, "CLI process");
    if (!child)
    {
        if (getenv("DVM_TEST_TERMINAL_CUSTOM"))
        {
            execl(runtime, runtime, "--terminal-launch-fd", descriptor, "provider/unibus-files.dis",
                "app.dis", "dep.dis", "--request", "job-request", "23",
                "--records", "job-records", "job-records", "--lane", "71", "--timeout-ms", "3000",
                "--grant", "/nine", "9", "r", "--grant", "/zero", "0", "rw", "--grant", "/seven", "7", "rw",
                "--fd-grant", "/input", "held-input", "r",
                "--fd-grant", "/output", "held-output", "w",
                "--", mode, bootstrap, (char *)NULL);
            _exit(3);
        }
        execl(runtime, runtime, "--terminal-launch-fd", descriptor, "provider/unibus-files.dis",
            "app.dis", "dep.dis", "--fd-grant", "/input", "held-input", "r",
            "--fd-grant", "/output", "held-output", "w", "--", mode, bootstrap, (char *)NULL);
        _exit(3);
    }
    close(fd);
    int status;
    launch_check(waitpid(child, &status, 0) == child && WIFEXITED(status), "CLI process exited");
    linkfixturefree();
    return WEXITSTATUS(status);
}


/* EN/RU: Require actual guest namespaces and FDs to release exports before joining native broker/physical owners.
 * Требовать освобождения exports настоящими guest namespaces/FD до join native broker/physical owners.
 */
int __wrap_main(int argc, char **argv)
{
    const char *runtime = getenv("DVM_TEST_TERMINAL_CLI");
    if (runtime && argc == 4)
    {
        int status = run_cli(runtime, argv[2], argv[3]);
        if (!status)
        {
            puts("PASS real Dis Unibus file native ownership drain");
        }
        return status;
    }
    DVM_LaunchHandoff *handoff = prepare_launch();
    int status = dvm_runtime_main_with_handoff(argc, argv, handoff);
    if (!status)
    {
        launch_check(!handoff->group && !handoff->flashvm.provider, "real pre-init ownership transfer");
    }
    dvm_launch_handoff_free(handoff);
    launch_check(execution.destroyed == 1 && terminal.destroyed == 1 && styxorderedfilesfree(files), "guest FD/provider lifecycle drain");
    styxclientshutdown(connection);
    styxclientfree(connection);
    linkfixturefree();
    puts("PASS real Dis Unibus file native ownership drain");
    return status;
}
