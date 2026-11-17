/* Copyright (c) 2026 bhgv
 * EN: Real host queue relay across a native original-9P launch handoff, without a fake UI queue.
 * RU: Настоящие host-очереди через native 9P handoff, без подмены локальных каналов.
 */
#include "bridge/bridge.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_UI_BRIDGE_CASE
#define STYX_UI_BRIDGE_CASE 0 /* Local Dis role, one complete UI exchange. */
#endif
#if STYX_UI_BRIDGE_CASE == 7
/* EN/RU: Joined TX-admission fault count from the test-only link wrapper.
 * Число попыток после join из тестового link-wrapper.
 */
extern unsigned terminalbridgefaultcalls(void);
#endif
#if STYX_UI_BRIDGE_CASE == 8
/* EN/RU: Exercise actual AS channel wrappers on their FlashVM owner, then retire that context.
 * Проверить настоящие AS-channel wrappers на своём FlashVM owner и завершить context.
 */
extern void terminalbridgeas(DVM_UIBridge *local, StyxBidirRoute *route);
#endif


/* EN/RU: Fail the native fixture explicitly in every compiler/build profile.
 * Явно завершить native-проверку при ошибке в любом профиле сборки.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal host bridge: %s\n", why);
        exit(2);
    }
}


/* EN: Observe a real blocked RX relay, bounded to about five seconds of native test time.
 * channel is borrowed; this is test observation, not production or guest polling.
 * RU: Дождаться настоящего заблокированного RX-потока, до пяти секунд native test time.
 * Заимствовать channel; это наблюдение теста, не production/guest-опрос.
 */
static void blocked_sender(FVM_Channel *channel)
{
    FVM_ChannelWaitState state; /* Mutex-protected actual FlashVM queue snapshot. */
    for (unsigned i = 0; i < 5000; i++)
    {
        check(fvm_channel_wait_state(channel, &state) == FVM_OK, "real queue state");
        if (state.waiting_senders)
        {
            return;
        }
        fvm_port_thread_sleep_us(1000);
    }
    check(0, "RX relay did not reach local backpressure");
}


/* EN: Borrow the actual client/descriptor, then own a relay through all endpoint operations.
 * Variants cover both local roles, full local RX cancellation, a rejected undersized route
 * and repeated TX frames. No Dis/FlashVM context or managed pointer crosses this boundary.
 * RU: Заимствовать client/descriptor и владеть мостом до завершения всех операций endpoint.
 * Проверить обе роли, отмену полного RX, малый лимит route и несколько TX-кадров.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Original launch authority, borrowed here. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    unsigned limit = STYX_UI_BRIDGE_CASE == 3 ? 256 : 2048; /* Known codec capacity requirement. */
    StyxBidirRoute *route = styxbidirbindingroute(client, &channels, limit, 1, "test.ide.fvd1", 1);
    check(route != NULL, "real owned terminal route");
    unsigned side = STYX_UI_BRIDGE_CASE == 1 || STYX_UI_BRIDGE_CASE == 8 ?
        DVM_UI_UI : DVM_UI_DIS; /* Retained local role. */
    StyxBidirUIBridge *bridge = styxbidiruibridge(route, side, 17); /* Consumes route on every result. */
    if (STYX_UI_BRIDGE_CASE == 3)
    {
        check(!bridge && styxclientalive(client), "undersized route refused without harming shared client");
        return;
    }
    check(bridge != NULL, "native host relay");
    DVM_UIBridge *local = styxbidiruibridgelocal(bridge); /* Borrowed real local queue pair. */
    route = styxbidiruibridgeroute(bridge);
    for (unsigned i = 0; i < 3; i++)
    {
        unsigned char bytes[256]; /* General records remain separate from the UI endpoint. */
        unsigned long tag; /* Copied numeric general route. */
        size_t size; /* Copied general payload bytes. */
        check(styxbidirroutewait(route, 0, 5000) &&
            styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD,
            "host consumes general events independently of the UI relay");
        check(tag == (i == 0 ? 0UL : i == 1 ? 5UL : 9UL), "general records preserve FIFO order");
    }
    if (STYX_UI_BRIDGE_CASE == 2)
    {
        FVM_Channel *outgoing; /* Borrowed local producer handle. */
        FVM_Channel *incoming; /* Borrowed local receive queue, filled by the native RX relay. */
        check(dvm_ui_channels(local, side, &outgoing, &incoming) == FVM_OK, "borrow real queue handles");
        blocked_sender(incoming);
        styxbidiruibridgejoin(bridge);
        DVM_UIMessage pending; /* Retained rejected RX copy survives the join boundary. */
        check(styxbidiruibridgepending(bridge, 0, &pending) && pending.id == 42,
            "blocked RX message survives stop/join for diagnosis");
        check(styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_STOPPED, "explicit stop wins relay races");
        styxbidiruibridgefree(bridge);
        check(styxclientalive(client), "full local queue teardown preserves shared client");
        return;
    }
#if STYX_UI_BRIDGE_CASE == 8
    terminalbridgeas(local, route);
    styxbidiruibridgejoin(bridge);
    styxbidiruibridgefree(bridge);
    check(styxclientalive(client), "AS wrapper and context teardown preserve the shared client");
    return;
#endif
    DVM_UIMessage message; /* Copied UI request, later reused for replies. */
    if (STYX_UI_BRIDGE_CASE == 5)
    {
        check(dvm_ui_receive(local, side, &message, 1) == FVM_ERROR &&
            styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE, "malformed remote UI closes local endpoint");
        styxbidiruibridgejoin(bridge);
        check(!styxbidiruibridgepending(bridge, 0, &message), "malformed UI never becomes a pending local message");
        styxbidiruibridgefree(bridge);
        check(styxclientalive(client), "remote relay failure preserves shared client");
        return;
    }
    check(dvm_ui_receive(local, side, &message, 1) == FVM_OK && message.id == 42 &&
        message.first == -2147483647L - 1 && message.size == 4 &&
        !memcmp(message.text, "A\0\320\257", 4), "automatic remote-to-real-local UI delivery");
    if (STYX_UI_BRIDGE_CASE == 6)
    {
        dvm_ui_close(local);
        check(!styxbidirroutewait(route, 1, 5000) &&
            styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_LOCAL, "local close cancels native relays automatically");
        styxbidiruibridgefree(bridge);
        check(styxclientalive(client), "local closure preserves unrelated remote session");
        return;
    }
    unsigned char declaration[] = {0x81, 0x80}; /* Independently established outgoing fixture profile. */
    unsigned char payload[240]; /* Known opaque TX fixture before any UI reply. */
    for (unsigned i = 0; i < sizeof(payload); i++)
    {
        payload[i] = (unsigned char)i;
    }
    check(styxbidirroutesend(route, 0, declaration, sizeof(declaration)) &&
        styxbidirrouteflush(route, 5000), "independent TX control");
    check(styxbidirroutesend(route, 101, payload, sizeof(payload)) &&
        styxbidirrouteflush(route, 5000), "independent TX binary data");
    message.kind = DVM_UI_REPLY;
    message.id = 43;
    message.first = 0;
    message.second = 0;
    message.size = 5;
    memcpy(message.text, "reply", 5);
    unsigned replies = STYX_UI_BRIDGE_CASE == 4 ? 8 : 1; /* Forces bounded one-slot TX pressure. */
    for (unsigned i = 0; i < replies; i++)
    {
        check(dvm_ui_send(local, side, &message, 1) == FVM_OK, "real local queue accepts copied reply");
    }
#if STYX_UI_BRIDGE_CASE == 7
    check(dvm_ui_receive(local, side, &message, 1) == FVM_ERROR &&
        styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE, "TX refusal closes the local endpoint");
    styxbidiruibridgejoin(bridge);
    check(styxbidiruibridgepending(bridge, 1, &message) && message.id == 43 &&
        message.size == 5 && !memcmp(message.text, "reply", 5), "unaccepted TX survives join unchanged");
    check(terminalbridgefaultcalls() == 1, "failed TX is never replayed or retried in a busy loop");
    styxbidiruibridgefree(bridge);
    check(styxclientalive(client), "TX relay failure preserves shared client");
    return;
#endif
    unsigned char bytes[32]; /* Independent remote completion receipt, not a queue-drain guess. */
    unsigned long tag; /* Receipt's general route. */
    size_t size; /* Receipt bytes. */
    check(styxbidirroutewait(route, 0, 10000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == 9 && size == 4 && !memcmp(bytes, "done", 4), "peer checked every relayed reply byte");
    styxbidiruibridgejoin(bridge);
    check(!styxbidiruibridgepending(bridge, 1, &message), "all locally accepted replies reached remote TX");
    styxbidiruibridgefree(bridge);
    check(styxclientalive(client), "native relay teardown preserves unrelated bidir traffic");
}
