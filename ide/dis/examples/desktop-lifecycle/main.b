implement LifecycleExample; # An ordinary Dis client of the portable pane service.
include "sys.m"; # Console output only; this example does not attach a GUI host.
include "uibridge.m"; # Copied key/text values, never native GUI objects.
include "idedeskpane.m"; # Each start creates an independently scheduled owner.
include "idedesktop.m"; # The separate owner holds layout values only.

LifecycleExample: module
{
    init: fn(); # Run the complete acknowledged close/recreate sequence.
};


# Exchange one request with a borrowed pane channel and consume its reply.
# operation chooses the public protocol; text/event are copied input values.
# The call blocks until the owner replies. Never call it again after Shutdown.
pane(owner: chan of Idedeskpane->Request, operation: int, text: string,
     event: ref Uibridge->Message): Idedeskpane->Reply
{
    reply := chan of Idedeskpane->Reply; # One caller owns this acknowledgement.
    owner <-= Idedeskpane->Request(1, operation, text, event, reply); # FIFO admission.
    return <-reply; # Retain the owner until the admitted operation completes.
}


# Exchange a copied layout request. owner remains borrowed and request receives
# a private reply endpoint; no document or MovieClip is acquired by this helper.
layout(owner: chan of Idedesktop->Request, request: Idedesktop->Request): Idedesktop->Reply
{
    request.reply = chan of Idedesktop->Reply; # Correlate the one outstanding call.
    owner <-= request; # A layout proposal never edits a source document.
    return <-request.reply; # Consume the terminal reply before reusing state.
}


# No parameters. Demonstrate document retirement independently of any renderer.
# Explicit checks raise on a failed contract. No file is written or compiled.
# The example chooses to discard its own generated text, not user source data.
init()
{
    sys := load Sys Sys->PATH; # Use the runtime's native console implementation.
    panes := load Idedeskpane Idedeskpane->PATH; # Resolve matching service bytecode.
    desk := load Idedesktop Idedesktop->PATH; # Resolve matching layout bytecode.
    if (sys == nil || panes == nil || desk == nil) { # Never silently skip the service.
        raise "fail: lifecycle dependencies"; # Report missing/incompatible modules.
    }
    first := panes->start(panes->Editor, "", "first"); # This process owns document A.
    second := panes->start(panes->Editor, "", "second"); # This process owns document B.
    if (first == nil || second == nil) { # A missing owner must not be used as a channel.
        raise "fail: lifecycle pane startup"; # No static document model is substituted.
    }
    changed := pane(first, panes->Input, "", ref Uibridge->Message(Uibridge->Text, 0, 0, 0, 0, 0, "π")); # Real edit.
    untouched := pane(second, panes->Snapshot, "", nil); # Observe B through its owner.
    if (changed.body != "πfirst" || changed.revision == changed.saved || untouched.body != "second") { # Check isolation.
        raise "fail: lifecycle independent documents"; # Dirty belongs only to A.
    }
    table := desk->start(); # The layout process owns no Idecore source buffer.
    request := Idedesktop->Request(1, desk->Update, 0, 1, 0, 0, 0, "A", changed.body, changed.caret, -1, 0, nil); # Slot A.
    view := layout(table, request); # Publish copied text, never the document heap.
    request.window = 4; # Keep B's stable identity distinct from A's.
    request.title = "B"; # Caption is presentation data only.
    request.body = untouched.body; # It cannot grant source-write authority.
    request.caret = untouched.caret; # Body-relative display position.
    view = layout(table, request); # Both layout windows now exist simultaneously.
    stopped := pane(first, panes->Shutdown, "", nil); # Explicitly discard example A; consume acknowledgement.
    if (stopped.status != 0) { # Removal must not conceal a failed owner shutdown.
        raise "fail: lifecycle shutdown"; # Do not reuse an unresolved endpoint.
    }
    first = nil; # The retired channel is terminal and is never sent another request.
    request.operation = desk->Remove; # Presentation retires after document ownership.
    request.window = 1; # Remove only A, leaving B and its state untouched.
    view = layout(table, request); # Scene omits A and its live miniature/close control.
    if (view.status != desk->Ok || view.focus != 4) { # The surviving editor takes focus.
        raise "fail: lifecycle layout retirement"; # No implicit global Quit is allowed.
    }
    first = panes->start(panes->Editor, "", "replacement"); # A fresh process reuses the free registry slot.
    if (first == nil) { # A new owner is mandatory; old text must not be resurrected.
        raise "fail: lifecycle replacement"; # Report allocation/dependency failure.
    }
    fresh := pane(first, panes->Snapshot, "", nil); # This is a new document, not an undo record.
    untouched = pane(second, panes->Snapshot, "", nil); # Recheck B after A's retirement.
    if (fresh.body != "replacement" || fresh.revision != fresh.saved || untouched.body != "second") { # Check reset/isolation.
        raise "fail: lifecycle fresh owner"; # History/path/dirty must not leak across ownership.
    }
    pane(first, panes->Shutdown, "", nil); # Retire the replacement and consume its terminal reply.
    pane(second, panes->Shutdown, "", nil); # Retire the still-independent surviving owner.
    request.operation = desk->Shutdown; # End the layout process after its clients.
    view = layout(table, request); # Final acknowledgement; table is now terminal too.
    if (view.status != desk->Ok) { # Never present an incomplete shutdown as success.
        raise "fail: lifecycle layout shutdown"; # Preserve observable failure.
    }
    sys->print("PASS desktop lifecycle tutorial: independent owners, terminal shutdown, fresh reuse\n"); # Real acceptance marker.
}
