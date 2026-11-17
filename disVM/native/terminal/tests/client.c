/* Copyright (c) 2026 bhgv
 * EN: General S-record transport on a real bidir launch handoff, one-record delivery and exact TX.
 * RU: Общий S-record transport на real bidir launch handoff, выдача по одной record и exact TX.
 */
#include "control.h"
#include "ui/frame.h"
#include "flashvm/sbindings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Explicit fatal fixture invariant in every build profile. / Явный fatal fixture invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal records: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Borrow the SAME client/descriptor after handoff; payload schema tags are fixture-only.
 * Borrow SAME client/descriptor после handoff; tags относятся только к fixture. */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    check(!styxbidircontrol(client, &channels, 0, 2) && !styxbidircontrol(client, &channels, 256, 33),
        "invalid limits refuse without protocol work");
    StyxBidirControl *control = styxbidircontrol(client, &channels, 256, 2);
    check(control != NULL, "bidir terminal transport");
    unsigned char declaration[] = {0x81, 0x80};
    unsigned char payload[256];
    for (unsigned i = 0; i < 240; i++)
    {
        payload[i] = (unsigned char)i;
    }
    check(!styxbidircontrolsend(control, 1, payload, 256), "oversized output refused atomically");
    check(styxbidircontrolsend(control, 0, declaration, sizeof(declaration)) &&
        styxbidircontrolsend(control, 101, payload, 240), "queued independent TX records");
    check(styxbidircontrolflush(control, 5000), "all short-write prefixes acknowledged exactly once");
    FVM_SBinding slots[2];
    FVM_SBindings bindings;
    check(fvm_sbindings_init(&bindings, slots, 2) == FVM_S_OK, "independent RX bindings");
    for (unsigned message = 0; message < 4; message++)
    {
        check(styxbidircontrolwait(control, 5000), "native record readiness without polling");
        unsigned long tag = 777;
        size_t size = 999;
        if (message < 2)
        {
            check(styxbidircontrolnext(control, &tag, NULL, 0, &size) == STYX_CONTROL_SMALL && tag == 777 && size > 0,
                "small buffer preserves complete head record");
        }
        check(styxbidircontrolnext(control, &tag, payload, sizeof(payload), &size) == STYX_CONTROL_RECORD,
            "one verified record per call");
        if (!message)
        {
            unsigned char encoded[256];
            size_t bytes, used;
            check(tag == 0 && fvm_srecord_write(0, payload, size, encoded, sizeof(encoded), &bytes) == FVM_S_OK &&
                fvm_sbindings_apply(&bindings, encoded, bytes, &used) == FVM_S_OK, "existing tag-zero binding dispatcher");
        }
        else if (message == 1)
        {
            FVM_SBinding binding;
            check(tag == 5 && size == 3 && payload[0] == 0 && payload[1] == 128 && payload[2] == 255 &&
                fvm_sbindings_lookup(&bindings, tag, &binding) == FVM_S_OK && !strcmp(binding.type, "test.opaque"),
                "binary payload uses existing independent binding table");
        }
        else if (message == 2)
        {
            check(tag == 9 && !size, "empty opaque record is still one event");
        }
        else
        {
            FVM_SBinding binding; /* Copied route identity from the control declaration. */
            DVM_UIMessage ui; /* Private copied IDE values, independent of either VM heap. */
            DVM_UIMessage before; /* Output sentinel for foreign/malformed routes. */
            memset(&ui, 0, sizeof(ui));
            before = ui;
            check(styxbidiruidecode(17, 0, payload, size, &ui) == STYX_UI_OTHER &&
                !memcmp(&ui, &before, sizeof(ui)), "control route is not consumed as UI");
            check(styxbidiruidecode(17, 17, "bad", 3, &ui) == STYX_UI_INVALID &&
                !memcmp(&ui, &before, sizeof(ui)), "malformed UI preserves copied output");
            check(fvm_sbindings_lookup(&bindings, tag, &binding) == FVM_S_OK &&
                !strcmp(binding.type, "test.ide.fvd1") &&
                styxbidiruidecode(17, tag, payload, size, &ui) == STYX_UI_MESSAGE &&
                ui.kind == DVM_UI_REQUEST && ui.id == 42 && ui.code == 77 &&
                ui.first == -2147483647L-1 && ui.second == 2147483647L &&
                ui.size == 4 && !memcmp(ui.text, "A\0\320\257", 4),
                "one routed fvd1 record retains int32, NUL and UTF-8 values");
            ui.kind = DVM_UI_REPLY;
            ui.id = 43;
            ui.first = 0;
            ui.second = 0;
            ui.size = 5;
            memcpy(ui.text, "reply", 5);
            check(!styxbidiruisend(control, 0, &ui), "tag-zero is never a UI route");
            check(styxbidiruisend(control, 17, &ui) && styxbidircontrolflush(control, 5000),
                "IDE reply uses existing native TX and exact short-write completion");
        }
    }
    check(styxbidircontrolstate(control) == STYX_CONTROL_LIVE, "idle receiver remains live");
    styxbidircontrolstop(control);
    check(!styxbidircontrolsend(control, 1, payload, 1), "stopped output refuses");
    styxbidircontrolfree(control);
    check(styxclientalive(client), "stop uses Tflush without closing shared bidir session");
}
