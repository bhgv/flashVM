/* Independent boundary tests for copied vector fragments, not a mirrored UI model. */
#include "desktop_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char valid[] = /* One red, unfilled two-node path in the logical canvas. */
    "00000001" "00e04444" "00000008" "00000000" "00000001" "00000000"
    "00000001" "00e04444" "00000008" "00000000" "00000002"
    "00000000" "00000064" "00000064" "00000000" "00000000"
    "00000001" "0000012c" "000000c8" "00000000" "00000000";


/* Preserve active evaluation in Release; label names the admission invariant. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "FAIL vector protocol: %s\n", label);
        exit(EXIT_FAILURE);
    }
}


/* Populate a copied envelope with borrowed literal bytes; no decoding is mocked. */
static void message(DVM_UIMessage *out, unsigned long code, const char *text)
{
    memset(out, 0, sizeof(*out));
    out->kind = DVM_UI_VIEW;
    out->code = code;
    out->flags = 7;
    out->size = strlen(text);
    memcpy(out->text, text, out->size);
}


/* Begin a genuine staged scene containing one vector-capable pane descriptor. */
static void scene(void)
{
    DVM_UIMessage out;
    dvm_desktop_destroy();
    message(&out, DVM_DESK_BEGIN, "00000007000000010000000000000064");
    out.first = 960;
    out.second = 640;
    require(dvm_desktop_message(&out), "scene begin");
    message(&out, DVM_DESK_ITEM,
            "000000010000006000000040000001a00000011000000001ffffffff00000000ffffffffVector");
    out.first = 1;
    require(dvm_desktop_message(&out), "copied pane");
}


/* Start the exact v2 vector extent after its owning pane has been staged. */
static void begin(void)
{
    DVM_UIMessage out;
    scene();
    message(&out, DVM_DESK_VECTOR_BEGIN, "0000000100000002");
    out.first = 1;
    out.second = (long)strlen(valid);
    require(dvm_desktop_message(&out), "vector exact extent declaration");
}


/* Reject a complete malformed stream and prevent End from admitting partial geometry.
 * offset selects a canonical word; replacement has exactly eight ASCII bytes. */
static void refuse(size_t offset, const char *replacement)
{
    DVM_UIMessage out;
    char data[sizeof(valid)];
    begin();
    memcpy(data, valid, sizeof(valid));
    memcpy(data + offset, replacement, 8);
    message(&out, DVM_DESK_VECTOR_DATA, data);
    out.first = 1;
    require(!dvm_desktop_message(&out), "malformed complete stream refused");
    message(&out, DVM_DESK_END, "");
    require(!dvm_desktop_message(&out), "incomplete/rejected geometry cannot commit");
}


/* Validate the only admitted nested forest: document -> container -> table.
 * Staged roots must precede children so presentation never observes a nil parent.
 * No FlashVM object is allocated by these decoder-only refusal controls. */
static void mdi_parents(void)
{
    DVM_UIMessage out;
    dvm_desktop_destroy();
    message(&out, DVM_DESK_BEGIN, "00000007000000010000000000000064");
    out.first = 960;
    out.second = 640;
    require(dvm_desktop_message(&out), "nested missing-root begin");
    message(&out, DVM_DESK_ITEM,
            "00000005000000080000002800000168000000fc00000000ffffffff00000000ffffffffchild");
    out.first = 1;
    require(dvm_desktop_message(&out), "staged bounded nested child");
    message(&out, DVM_DESK_END, "");
    require(!dvm_desktop_message(&out), "nested child without its root refused before commit");
    dvm_desktop_destroy();
    message(&out, DVM_DESK_BEGIN, "00000007000000020000000000000064");
    out.first = 960;
    out.second = 640;
    require(dvm_desktop_message(&out), "valid nested begin");
    message(&out, DVM_DESK_ITEM,
            "000000010000006000000048000002e80000013400000004ffffffff00000000ffffffffSource MDI");
    out.first = 13;
    require(dvm_desktop_message(&out), "container root beneath table");
    message(&out, DVM_DESK_ITEM,
            "00000005000000080000002800000168000000fc00000000ffffffff00000000ffffffffchild");
    out.first = 1;
    require(dvm_desktop_message(&out), "child below preceding container");
    message(&out, DVM_DESK_END, "");
    require(dvm_desktop_message(&out), "complete bounded nested forest admitted");
}


/* Check fragmentation, ticket/order, immutable extent bounds and command validation. */
int main(void)
{
    DVM_UIMessage out;
    mdi_parents();
    scene();
    message(&out, DVM_DESK_VECTOR_BEGIN, "00000001");
    out.first = 1;
    out.second = (long)strlen(valid);
    require(!dvm_desktop_message(&out), "old unversioned Begin is not silently reinterpreted");
    message(&out, DVM_DESK_VECTOR_BEGIN, "0000000100000003");
    out.first = 1;
    out.second = (long)strlen(valid);
    require(!dvm_desktop_message(&out), "unknown vector version");
    begin();
    message(&out, DVM_DESK_END, "");
    require(!dvm_desktop_message(&out), "missing vector fragments block commit");
    message(&out, DVM_DESK_VECTOR_DATA, valid);
    out.first = 1;
    out.flags = 6;
    require(!dvm_desktop_message(&out), "stale ticket");
    out.flags = 7;
    out.second = 1;
    require(!dvm_desktop_message(&out), "out-of-order offset");
    out.second = 0;
    out.size = 0;
    require(!dvm_desktop_message(&out), "empty fragment");
    out.size = 17;
    require(dvm_desktop_message(&out), "split within a word is assembled without partial parsing");
    message(&out, DVM_DESK_VECTOR_DATA, valid + 17);
    out.first = 1;
    out.second = 17;
    require(dvm_desktop_message(&out), "complete validated geometry");
    require(!dvm_desktop_message(&out), "duplicate final fragment");
    message(&out, DVM_DESK_END, "");
    require(dvm_desktop_message(&out), "complete scene commit");
    refuse(0, "00000000");
    refuse(8, "ffffffff");
    refuse(16, "00000000");
    refuse(24, "00000002");
    refuse(32, "00000011");
    refuse(40, "00000011");
    refuse(48, "00000021");
    refuse(56, "ffffffff");
    refuse(64, "00000021");
    refuse(72, "00000002");
    refuse(80, "ffffffff");
    refuse(88, "00000001");
    refuse(96, "000003e9");
    refuse(104, "00000259");
    refuse(112, "00000001");
    refuse(128, "00000000");
    refuse(128, "00000004");
    refuse(128, "gggggggg");
    dvm_desktop_destroy();
    puts("PASS vector protocol: fragments, stale/order/quotas, full command validation, no partial commit");
    return 0;
}
