/* Copyright (c) 2026 bhgv
 * EN: Actual launch-ticket handoff to shared ordered attachment, independent of a display and caller owners.
 * RU: Настоящий launch-ticket handoff в общий ordered attachment, независимо от дисплея и исходных owners.
 */
#include "../../attach.h"
#include "mount.h"
#include "request.h"
#include "../../../../../tests/launch/backend.h"


/* EN/RU: Own actual original Styx client; physical pumps survive until fixturefree.
 * Свой настоящий Styx-client, physical pumps живут до fixturefree.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Join original server and physical owners after all protocol users retire.
 * Завершить исходный server и physical owners после всех protocol users.
 */
extern void linkfixturefree(void);


/* EN/RU: Validate malformed options before any control construction; actual client and root stay healthy.
 * Проверить неверные options до создания control, настоящий client/root остаются живыми.
 */
extern void attachmentpreflight(StyxClient *, const DVM_TerminalChannels *, const StyxOrderedOptions *);


/* EN/RU: Copy one independently ordered inbox record; peer remains pinned through return.
 * Скопировать одну запись своего inbox; peer удерживается до возврата.
 */
static void take(StyxOrdered *peer, unsigned long tag, unsigned long long expected, const char *text)
{
    StyxOrderedClient *client = styxorderedclient(peer, tag); /* Fixed authorized inbox. */
    char bytes[32]; /* Private application payload. */
    size_t size; /* Exact copied bytes. */
    unsigned long long ordinal; /* Private sequence checkpoint. */
    launch_check(client && styxorderedclientwait(client, 3000) &&
        styxorderedclientnext(client, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD &&
        ordinal == expected && size == strlen(text) && !memcmp(bytes, text, size), "captured attachment receive");
}


/* EN/RU: Actual native ticket prepare/take, no fabricated namespace pointer or guest/GUI execution.
 * Настоящие native ticket prepare/take, без фиктивного namespace и guest/GUI-кода.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Own original session independently of launch and broker. */
    launch_check(client != NULL, "actual original client");
    LaunchState terminal = {0}; /* Local unused preparation-root fixture, disposed before remote request. */
    LaunchState execution = {0}; /* Live provider state through all ticket/namespace references. */
    DVM_LaunchOptions environment = launch_options(&terminal, &execution); /* Own authorized execution source. */
    dvm_provider_release(environment.terminal);
    environment.terminal = NULL;
    unsigned char arguments[] = {0, 1, 255, 2}; /* Immutable request bytes copied by ticket. */
    DVM_LaunchSpec spec = {0}; /* Existing native launch description, not a network launch RPC. */
    spec.policy = environment.policy;
    spec.terminal_origin = environment.terminal_origin;
    strcpy(spec.expected_provider, environment.provider_origin.system);
    strcpy(spec.module, "provider/main.dis");
    spec.arguments = arguments;
    spec.argument_bytes = sizeof(arguments);
    StyxTerminalPaths paths = {"records", "records", NULL}; /* Exact same granted remote root for launch and transport. */
    DVM_LaunchRequest *request = NULL; /* Separate owned provider authority. */
    launch_check(styxclientretain(client), "additional terminal session pin");
    DVM_LaunchTicket *ticket = styxbidirlaunchrequest(client, &spec, FVM_ACCESS_MASK, &paths, &request); /* Consumes extra pin. */
    launch_check(ticket && request && dvm_launch_request_prepare(request, environment.provider, &environment.provider_origin),
        "real remote-root request preparation");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request); /* One-use actual owned namespace/channel transfer. */
    launch_check(handoff && handoff->group && handoff->flashvm.provider &&
        !memcmp(handoff->arguments, arguments, sizeof(arguments)), "actual channel-bearing handoff");
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(environment.provider);
    unsigned long tags[] = {0, 7, 9}; /* Fixed independent application grants, inner0 stays private. */
    StyxOrderedOptions options = {sizeof(options), 128, 16, 71, tags, 3}; /* Generic display-free launch profile. */
    attachmentpreflight(client, &handoff->flashvm, &options);
    DVM_LaunchHandoff before = *handoff; /* Diagnostic bytes only, not a second owning copy. */
    handoff->description.terminal_channels = 0;
    launch_check(!styxbidirorderedlaunch(client, handoff, &options), "console handoff never constructs a display");
    handoff->description.terminal_channels = before.description.terminal_channels;
    StyxOrdered *peer = styxbidirorderedlaunch(client, handoff, &options); /* Owns its own captured fids. */
    launch_check(peer && !memcmp(handoff, &before, sizeof(before)), "attachment does not mutate or consume handoff");
    tags[0] = 99; /* Caller configuration may disappear immediately after construction. */
    options.limit = 1;
    dvm_launch_handoff_free(handoff);
    launch_check(execution.destroyed == 1, "execution namespace released independently of terminal broker");
    launch_check(styxorderedwait(peer, STYX_ORDER_TX, 3000) && styxorderedwait(peer, STYX_ORDER_RX, 3000), "automatic initial offer");
    take(peer, 9, 0, "nine");
    take(peer, 9, 1, "rx-live");
    take(peer, 0, 0, "local-in");
    take(peer, 7, 0, "zero");
    take(peer, 7, 1, "one");
    take(peer, 7, 2, "two");
    take(peer, 7, 0, "new");
    launch_check(!styxorderedclient(peer, 99), "grant array copied before publication");
    styxorderedfree(peer);
    StyxFid *fid = styxclientwalk(client, "records"); /* Unrelated caller RPC remains usable after owned broker disposal. */
    launch_check(fid != NULL && styxclientalive(client), "caller session survives attachment disposal");
    styxclientclunk(fid);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    return 0;
}
