/* Copyright (c) 2026 bhgv
 * EN: Native terminal channels stay inside the actual captured grant and its ceiling, never ambient or foreign roots.
 * RU: Native terminal channels остаются внутри captured grant/ceiling, не ambient либо foreign roots.
 */
#include "control.h"
#include "mount_grant.h"
#include "cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_GRANT_CASE
#define STYX_GRANT_CASE 0 /* Scoped directory with colliding ambient endpoint. */
#endif


#ifdef STYX_GRANT_RESOURCE_TEST


/* EN/RU: Actual announced resource lookup under the same directory/file grant, no ambient service fallback.
 * Настоящий announced resource lookup в directory/file grant, без ambient service fallback.
 */
extern void terminalgrantresource(StyxClient *, const DVM_TerminalChannels *, int);
#endif


/* EN/RU: Atomic original-server callback counter for one endpoint, no I/O or ownership transfer.
 * Atomic counter original-server callbacks одного endpoint, без I/O/передачи ownership.
 */
extern unsigned terminalgrantcalls(unsigned index);


/* EN/RU: Owned independent original native 9P client/NULL; caller consumes after all its grant references retire.
 * Свой independent native 9P client/NULL; поглотить после release всех его grant refs.
 */
extern StyxClient *terminalgrantforeign(void);


/* EN/RU: Consume the independent foreign client owner and join its original fixture server.
 * Поглотить independent foreign client owner и join original fixture server.
 */
extern void terminalgrantforeignfree(StyxClient *);


/* EN/RU: Owned generic PATH-capable provider/NULL, deliberately unsupported by the Styx-only native adapter.
 * Свой generic PATH-capable provider/NULL, намеренно unsupported для Styx-only adapter.
 */
extern DVM_Provider *terminalgrantunsupported(void);


/* EN/RU: Callback sentinel count, proves unsupported provider was rejected without being probed or opened.
 * Callback sentinel count проверяет отказ без probe/open unsupported provider.
 */
extern unsigned terminalgrantunsupportedcalls(void);


/* EN/RU: Arm constructor-owner cancellation immediately before the next root-clone RPC admission.
 * Включить constructor-owner cancellation непосредственно до следующего root-clone RPC admission.
 */
extern void terminalgrantcancelarm(void);


/* EN/RU: Explicit fixture check, active in every build profile; why borrowed until return/exit.
 * Явная fixture check во всех build profiles; why borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal channel grant: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Retain caller client before constructor consumption; return owned captured grant/NULL.
 * Retain client caller до consuming constructor; вернуть owned captured grant/NULL.
 */
static DVM_Provider *captured(StyxClient *client, const char *path, unsigned ceiling)
{
    check(styxclientretain(client), "caller client pin before captured grant");
    return styxmountproviderat(client, path, ceiling);
}


/* EN/RU: Live fixture mutation uses an independent explicitly opened ctl fid, never terminal-grant fallback.
 * Live fixture mutation через отдельный явно открытый ctl fid, не terminal-grant fallback.
 */
static void command(StyxClient *client, const char *text)
{
    StyxFid *fid = styxclientwalk(client, "grantctl"); /* Explicit trusted fixture administration channel. */
    check(fid && styxclientopen(fid, 1), "fixture control channel");
    StyxTransfer result; /* Original precise write acknowledgement. */
    styxclientwrite(fid, 0, text, strlen(text), &result);
    check(result.success && result.confirmed && !result.uncertain && result.count == strlen(text), "live fixture control acknowledged");
    styxclientclunk(fid);
}


/* EN/RU: Read one exact S-record, preserving all other pending work; caller pins control through return.
 * Прочитать одну exact S-record, сохраняя прочую pending work; caller удерживает control до return.
 */
static void record(StyxBidirControl *control, unsigned long expected, const char *label)
{
    unsigned char bytes[64]; /* One copied record, never a batch. */
    unsigned long tag; /* Exact source tag. */
    size_t size; /* Exact copied body length. */
    check(styxbidircontrolwait(control, 5000) &&
        styxbidircontrolnext(control, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected &&
        size == strlen(label) && !memcmp(bytes, label, size), "captured grant identity, not ambient path identity");
}


/* EN/RU: Exercise the existing launch handoff alongside separately captured terminal grants on the same real client.
 * Проверить existing launch handoff и отдельно captured terminal grants того же настоящего client.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    const unsigned scenario = STYX_GRANT_CASE; /* Compile-time deterministic grant variant. */
    DVM_TerminalChannels channels = *descriptor; /* Borrow launch descriptor, replace only the caller's local copy. */
    channels.provider = NULL;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    const char *label = scenario == 1 || scenario == 14 ? "nested" : "scoped"; /* Independent remote byte oracle. */
    const char *path = scenario == 1 ? "grant/nested" : scenario == 2 || scenario == 9 || scenario == 19 ? "grant/records" : "grant";
    unsigned ceiling = scenario == 4 ? FVM_ACCESS_READ | FVM_ACCESS_EXEC : scenario == 5 ?
        FVM_ACCESS_WRITE | FVM_ACCESS_EXEC : scenario == 6 || scenario == 2 || scenario == 19 ?
        FVM_ACCESS_READ | FVM_ACCESS_WRITE : FVM_ACCESS_MASK; /* Fixed local grant policy. */
    channels.provider = captured(client, path, ceiling);
    check(channels.provider != NULL, "owned captured native provider");
    if (scenario == 2 || scenario == 19)
    {
        strcpy(channels.receive, ".");
        strcpy(channels.send, ".");
    }
    if (scenario == 3)
    {
        command(client, "rebind");
        DVM_Provider *replacement = captured(client, "grant", FVM_ACCESS_MASK); /* New pathname resolves a different Qid. */
        FVM_FileInfo old_info; /* Captured root identity remains the moved original. */
        FVM_FileInfo new_info; /* Replacement is a distinct live node. */
        check(replacement && dvm_provider_info(channels.provider, ".", &old_info) == FVM_OK &&
            dvm_provider_info(replacement, ".", &new_info) == FVM_OK && old_info.identity.low != new_info.identity.low,
            "old grant survives pathname replacement with distinct native identity");
        dvm_provider_release(replacement);
    }
    if (scenario == 14)
    {
        DVM_Provider *child = dvm_provider_capture(channels.provider, "nested"); /* Independent retained child root. */
        dvm_provider_release(channels.provider);
        channels.provider = child;
        check(child != NULL, "captured child retains source/root ownership after parent release");
    }
    StyxClient *foreign = NULL; /* Optional independently owned second original client. */
    if (scenario == 7 || scenario == 11)
    {
        dvm_provider_release(channels.provider);
        if (scenario == 7)
        {
            channels.provider = terminalgrantunsupported();
        }
        else
        {
            foreign = terminalgrantforeign();
            check(foreign != NULL, "second real native original client");
            channels.provider = captured(foreign, ".", FVM_ACCESS_MASK);
        }
        check(channels.provider && dvm_terminal_channels_valid(&channels), "descriptor validity alone supplies no matching-client authority");
    }
    if (scenario == 8)
    {
        strcpy(channels.receive, "../records");
    }
    if (scenario == 9)
    {
        strcpy(channels.send, ".");
    }
    if (scenario == 10)
    {
        strcpy(channels.receive, "readonly");
        strcpy(channels.send, "readonly");
    }
    if (scenario == 15)
    {
        strcpy(channels.receive, "nested");
        strcpy(channels.send, "nested");
    }
    if (scenario == 16)
    {
        strcpy(channels.receive, "input");
        strcpy(channels.send, "output");
    }
    if (scenario == 17)
    {
        command(client, "readonly");
    }
    int refusal = (scenario >= 4 && scenario <= 13) || scenario == 15 || scenario == 17; /* Invalid/denied paths must never fallback. */
    DVM_Cancel *cancel = scenario == 12 || scenario == 13 ? dvm_cancel_create() : NULL; /* Constructor-only caller cancellation. */
    DVM_CancelScope scope = {0}; /* Original native owner until setup returns. */
    if (cancel)
    {
        check(dvm_cancel_enter(&scope, cancel), "constructor current cancellation scope");
        if (scenario == 12)
        {
            dvm_cancel_request(cancel);
        }
        else
        {
            terminalgrantcancelarm();
        }
    }
#ifdef STYX_GRANT_RESOURCE_TEST
    terminalgrantresource(client, &channels, scenario == 19);
    dvm_provider_release(channels.provider);
    return;
#endif
    StyxBidirControl *control = styxbidircontrol(client, &channels, 256, 2); /* Must derive both endpoints from this grant. */
    if (cancel)
    {
        check(dvm_cancel_requested(cancel) && dvm_cancel_leave(&scope), "observed constructor cancellation restores caller scope");
        dvm_cancel_release(cancel);
    }
    if (refusal)
    {
        check(!control && !terminalgrantcalls(0) && styxclientalive(client),
            "grant/ceiling/path/permission refusal cannot probe or open ambient endpoint");
        if (scenario == 7)
        {
            check(!terminalgrantunsupportedcalls(), "unknown factory rejected without generic callbacks");
        }
        dvm_provider_release(channels.provider);
        channels.provider = captured(client, scenario == 3 ? "moved" : "grant", FVM_ACCESS_MASK);
        strcpy(channels.receive, "records");
        strcpy(channels.send, "records");
        if (scenario == 17)
        {
            command(client, "writable");
        }
        control = styxbidircontrol(client, &channels, 256, 2);
    }
    check(control != NULL, "captured grant admits its permitted native channel pair");
    if (scenario == 14)
    {
        dvm_provider_release(channels.provider);
        channels.provider = NULL;
    }
    StyxFid *outside = styxbidircontrolwalk(control, "identity"); /* Ambient sibling exists but must not be reached through this grant. */
    check(!outside && !styxbidircontrolwalk(control, "../identity") &&
        !styxbidircontrolwalk(control, "/identity"), "root-relative walks never acquire ambient or escaping names");
    record(control, 41, label);
    check(styxbidircontrolsend(control, 42, "reply", 5) && styxbidircontrolflush(control, 5000), "scoped endpoint accepts exact native TX prefixes");
    record(control, 43, label);
    styxbidircontrolfree(control);
    dvm_provider_release(channels.provider);
    if (foreign)
    {
        terminalgrantforeignfree(foreign);
    }
    check(!terminalgrantcalls(0) && styxclientalive(client), "ambient endpoint untouched and ordinary original client still alive");
}
