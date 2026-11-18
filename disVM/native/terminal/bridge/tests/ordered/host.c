/* Copyright (c) 2026 bhgv
 * EN: Captured launch descriptor drives real local channels over original Styx and the negotiated broker.
 * RU: Исходный launch descriptor подключает реальные локальные каналы к исходному Styx и согласованному брокеру.
 */
#include "bridge/ordered.h"
#include "mount.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Own original fixture client/NULL; physical owner survives until explicit join.
 * Получить свой исходный fixture client/NULL; physical owner существует до join.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Break actual native carrier; no simulated broker notification.
 * Разорвать настоящий native-carrier, без подмены уведомления брокера.
 */
extern void linkfixturebreak(void);


/* EN/RU: Join original server/pumps after all client users retire.
 * Завершить исходный сервер/pumps после всех client users.
 */
extern void linkfixturefree(void);


/* EN/RU: Test-only post-free delayed callbacks; state remains fixture-owned until this call.
 * Тестовые callbacks после free; state остаётся у fixture до этого вызова.
 */
extern void orderedterminallate(void);


/* EN/RU: Fixture constructor-call count for invalid options, no transport operation.
 * Число вызовов конструктора при неверных options; без операций транспорта.
 */
extern unsigned orderedterminalcalls(void);


/* EN/RU: Arm test-only worker creation failure after original shared client is fully constructed.
 * Включить test-only отказ создания worker после полного создания исходного общего клиента.
 */
extern void orderedterminalarm(void);


/* EN/RU: Native receive/send entry checkpoints while dispatcher is deliberately held by the fixture.
 * Native-checkpoints входа RX/TX, пока fixture намеренно удерживает dispatcher.
 */
extern void orderedterminalblocked(void);


/* EN/RU: Release held native dispatcher after actual retirement has been observed.
 * Отпустить удержанный native-dispatcher после наблюдения настоящего завершения.
 */
extern void orderedterminalrelease(void);


/* EN/RU: Actual FlashVM AS wrappers copy values; owner is destroyed before native TX is released.
 * Реальные FlashVM AS-wrappers копируют значения; owner удаляется до освобождения native TX.
 */
extern void orderedterminalas(DVM_UIBridge *local);


/* EN/RU: Fatal explicit contract with source line, active under optimization.
 * Явная обязательная проверка с номером строки, активна при оптимизации.
 */
static void require(int condition, int line)
{
    if (!condition)
    {
        fprintf(stderr, "ordered terminal case %d line %d\n", ORDERED_TERMINAL_CASE, line);
        exit(2);
    }
}
#define CHECK(value) require((value) != 0, __LINE__) /* Always evaluated contract. */


/* EN/RU: Copy one independent native client receipt; never consume the UI relay inbox.
 * Прочитать одну независимую native-квитанцию; UI-очередь relay не потребляется.
 */
static void other(StyxOrdered *peer, unsigned long long expected, const char *text)
{
    StyxOrderedClient *client = styxorderedclient(peer, 9); /* Distinct fixed-tag native capability. */
    unsigned long long ordinal; /* Copied full number. */
    char bytes[64]; /* Independent payload copy. */
    size_t size; /* Copied extent. */
    CHECK(client && styxorderedclientwait(client, 5000));
    CHECK(styxorderedclientnext(client, &ordinal, bytes, sizeof(bytes), &size) == STYX_ORDER_RECORD);
    CHECK(ordinal == expected && size == strlen(text) && !memcmp(bytes, text, size));
}


/* EN/RU: Observe actual blocked local RX sender, native test only, never production/guest polling.
 * Наблюдать настоящую блокировку локального RX-отправителя; только native-тест, не production/guest polling.
 */
static void full(DVM_UIBridge *local, unsigned side)
{
    FVM_Channel *outgoing; /* Borrowed local producer. */
    FVM_Channel *incoming; /* Borrowed local consumer. */
    CHECK(dvm_ui_channels(local, side, &outgoing, &incoming) == FVM_OK);
    for (unsigned i = 0; i < 5000; i++)
    {
        FVM_ChannelWaitState state; /* Actual native channel mutex snapshot. */
        CHECK(fvm_channel_wait_state(incoming, &state) == FVM_OK);
        if (state.waiting_senders)
        {
            return;
        }
        fvm_port_thread_sleep_us(1000);
    }
    CHECK(0);
}


/* EN/RU: Execute one real terminal launch/teardown scenario, retaining ordinary client/fids independently.
 * Выполнить настоящий запуск/завершение терминала, независимо сохраняя обычный client/fids.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Owned native original connection. */
    CHECK(client && styxclientretain(client));
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Captured VFS authority, consumes extra pin. */
    DVM_TerminalChannels channels = {0}; /* Borrowed captured terminal paths through construction. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    unsigned side = ORDERED_TERMINAL_CASE == 1 || ORDERED_TERMINAL_CASE == 5 ? DVM_UI_UI : DVM_UI_DIS; /* Local owner role. */
    unsigned long ui_tag = ORDERED_TERMINAL_CASE == 1 ? 0 : 7; /* Test application-local zero too. */
    unsigned long tags[] = {ui_tag, 9}; /* Explicit UI plus independent resource-like inbox. */
    StyxOrderedTerminalOptions options = {sizeof(options), side, 2048, 8, 71, ui_tag, tags, 2}; /* Native-only launch options. */
    orderedterminalarm();
    StyxBidirUIBridge *bridge = NULL; /* Owned adapter, optional for non-display paths. */
    if (ORDERED_TERMINAL_CASE == 6)
    {
        StyxOrderedTerminalOptions invalid = options; /* Invalid metadata must refuse before network/setup. */
        invalid.limit = 256;
        CHECK(!styxbidirorderedterminal(client, &channels, &invalid));
        invalid = options;
        invalid.ui_tag = 99;
        CHECK(!styxbidirorderedterminal(client, &channels, &invalid));
        CHECK(!styxbidirorderedterminal(client, &channels, NULL));
        CHECK(!orderedterminalcalls());
    }
    else if (ORDERED_TERMINAL_CASE == 8)
    {
        StyxOrdered *peer = styxbidirordered(styxbidircontrol(client, &channels, 2112, 1), 2048, 8, tags, 2); /* Owned manual profile. */
        CHECK(peer);
        styxorderedstop(peer);
        bridge = styxbidirordereduibridge(peer, side, ui_tag); /* Sticky replay closes local channels before relay use. */
        CHECK(bridge && styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE);
        DVM_UIMessage message; /* Empty closed local endpoint. */
        CHECK(dvm_ui_receive(styxbidiruibridgelocal(bridge), side, &message, 1) == FVM_ERROR);
    }
    else
    {
        bridge = styxbidirorderedterminal(client, &channels, &options);
        if (ORDERED_TERMINAL_CASE == 7 || ORDERED_TERMINAL_CASE >= 12)
        {
            CHECK(!bridge); /* Rejected watch setup consumes only partial owned resources. */
        }
        else
        {
            CHECK(bridge && !styxbidiruibridgeroute(bridge));
            DVM_UIBridge *local = styxbidiruibridgelocal(bridge); /* Existing channel API, borrowed through free. */
            StyxOrdered *peer = styxbidiruibridgeordered(bridge); /* Native host services only non-UI grants. */
            CHECK(peer && local);
            DVM_UIMessage message = {0}; /* Local copied frame, no managed cross-heap pointers. */
            if (ORDERED_TERMINAL_CASE == 11)
            {
                message.kind = DVM_UI_REPLY;
                message.id = 43;
                message.size = 5;
                memcpy(message.text, "reply", 5);
                CHECK(dvm_ui_send(local, side, &message, 1) == FVM_OK);
                orderedterminalblocked();
                linkfixturebreak();
                CHECK(dvm_ui_receive(local, side, &message, 1) == FVM_ERROR);
                CHECK(styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE);
                orderedterminalrelease();
            }
            else if (ORDERED_TERMINAL_CASE == 4)
            {
                CHECK(dvm_ui_receive(local, side, &message, 1) == FVM_ERROR);
                CHECK(styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE);
                styxbidiruibridgejoin(bridge);
                CHECK(!styxbidiruibridgepending(bridge, 0, &message));
            }
            else
            {
                other(peer, 0, "independent");
                if (ORDERED_TERMINAL_CASE == 2)
                {
                    full(local, side);
                    linkfixturebreak();
                    while (dvm_ui_receive(local, side, &message, 1) == FVM_OK)
                    {
                    }
                    CHECK(styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE);
                }
                else if (ORDERED_TERMINAL_CASE == 5)
                {
                    orderedterminalas(local);
                    other(peer, 1, "done");
                }
                else
                {
                    CHECK(dvm_ui_receive(local, side, &message, 1) == FVM_OK && message.id == 42 &&
                        message.size == 4 && !memcmp(message.text, "A\0\320\257", 4));
                    message.kind = DVM_UI_REPLY;
                    message.id = 43;
                    message.first = 0;
                    message.size = ORDERED_TERMINAL_CASE == 3 ? 300 : 5;
                    memset(message.text, 'x', message.size);
                    if (ORDERED_TERMINAL_CASE != 3)
                    {
                        memcpy(message.text, "reply", 5);
                    }
                    CHECK(dvm_ui_send(local, side, &message, 1) == FVM_OK);
                    if (ORDERED_TERMINAL_CASE == 3)
                    {
                        CHECK(dvm_ui_receive(local, side, &message, 1) == FVM_ERROR);
                        CHECK(styxbidiruibridgestate(bridge) == STYX_UI_BRIDGE_REMOTE);
                        styxbidiruibridgejoin(bridge);
                        CHECK(styxbidiruibridgepending(bridge, 1, &message) && message.size == 300);
                    }
                    else
                    {
                        other(peer, 1, "done");
                        CHECK(styxorderedservicewait(peer, 3000));
                        char bytes[16]; /* Root native notice, never delivered through the UI tag0. */
                        size_t size; /* Exact service extent. */
                        CHECK(styxorderedservicenext(peer, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && size == 6 && !memcmp(bytes, "native", 6));
                    }
                }
            }
        }
    }
    styxbidiruibridgefree(bridge);
    orderedterminallate();
    if (ORDERED_TERMINAL_CASE != 2 && ORDERED_TERMINAL_CASE != 11)
    {
        CHECK(styxclientalive(client));
        StyxFid *fid = styxclientwalk(client, "records"); /* Ordinary file access survives optional/failed/closed display setup. */
        StyxMeta metadata; /* Copied original Tstat result. */
        CHECK(fid && styxclientstat(fid, &metadata));
        styxclientclunk(fid);
    }
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    return 0;
}
