implement GraphicalEditor; # Execute document policy on the real Dis VM.
include "sys.m"; # Use actual native file I/O through the common FlashVM ports.
include "uibridge.m"; # Exchange copied, bounded envelopes with the renderer.
include "idecore.m";
include "idevalue.m";
include "ideprojection.m";
include "idedocprojection.m"; # Keep text and history in their separate channel owner.
include "ideinput.m"; # Reuse Dis keyboard, selection and revision policy.
include "idebridge.m"; # Reuse the bounded snapshot serializer.
include "idefileops.m"; # Reuse bounded source I/O independently of any renderer.
include "idesearch.m"; # Reuse literal document operations through typed channels.
include "ideviewport.m"; # Keep viewport decisions and bounds in a separate Dis module.
include "idefield.m"; # Reuse single-line value/selection policy for every modal field.
include "ideclipboard.m"; # Keep copied clipboard fragments in an exclusive Dis staging owner.
include "idefiles.m"; # Own native directory cursors through typed requests.
include "idebrowser.m"; # Keep browser paging/selection in exclusive Dis state.
include "ideworkspace.m"; # Compiler snapshots have a separate private owner.
include "fircompiler.m"; # Select explicit library language/target values.
include "idecompiler.m"; # Build jobs remain Limbo channel operations.
include "idebuildpanel.m"; # Bind artifacts and diagnostics to this document revision.
include "idehost.m"; # Register ordinary channels with the native window owner.

GraphicalEditor: module # Parameterless entry accepted by the experimental host.
{
    init: fn(); # Own the controller until acknowledged clean/forced shutdown.
};

directoryfiles: Idefiles; # Loaded directory owner, separate from document-file operations.
browsing: Idebrowser; # Reusable Dis-only browser state and channel adapter.
sys: Sys; # Loaded file services, exclusive to this controller's Dis process.
core: Idecore; # Loaded document implementation; no native text authority.
codec: Uibridge; # Shared value codec, never a native pointer transport.
files: Idefileops; # Reusable checked source-file library, called only by Dis policy.
search: Idesearch; # Reusable search/replacement library, using actual document channels.
bridge: Idebridge; # Existing ordered Unicode-fragment serializer.


# Read a bounded metadata snapshot from the actual document projection owner.
# endpoint is borrowed; blocks this controller only. No renderer evaluates or reads document state.
inspectdocument(endpoint: chan of ref Ideprojection->Request): string
{
    answers := chan of Ideprojection->Reply;
    endpoint <-= ref Ideprojection->Request(1, 1, Ideprojection->ToTable, "", 0, "", 0, 0,
                                          Idevalue->Table, nil, answers);
    result := <-answers;
    if (result.status != Ideprojection->Ok || result.value == nil || len result.value.names != 5) {
        return "unavailable status=" + string result.status;
    }
    text := "";
    for (i := 0; i < 4; i++) {
        if (i != 0) {
            text += " ";
        }
        text += result.value.names[i] + "=" + string result.value.values[i].integer;
    }
    return text;
}


# Retire the projection adapter before shutting down the borrowed document endpoint.
# endpoint is borrowed; consumes its terminal acknowledgement or raises a lifecycle failure.
releaseprojection(endpoint: chan of ref Ideprojection->Request)
{
    answers := chan of Ideprojection->Reply;
    endpoint <-= ref Ideprojection->Request(2, 1, Ideprojection->Release, "", 0, "", 0, 0,
                                          0, nil, answers);
    result := <-answers;
    if (result.status != Ideprojection->Ok) {
        raise "fail: graphical document projection release";
    }
}


# Send one synchronous operation to the separate document owner and receive its result.
# Arguments are values/borrowed channels; blocks only this Limbo process.
call(document: chan of Idecore->Request, operation: int, revision: int, text: string): Idecore->Reply
{
    answers := chan of Idecore->Reply; # Private rendezvous destination for this call.
    document <-= Idecore->Request(1, operation, revision, 0, 0, text, "", answers); # Copy the request.
    return <-answers; # Return the acknowledged value snapshot.
}


# Read a bounded UTF-8 source from path, returning (success, text); no descriptors escape.
# Errors/oversize/invalid UTF-8 preserve the existing document; reads may block the host owner.
readsource(path: string): (int, string)
{
    (status, text) := files->read(sys, path, 65536); # Validate bytes and close the local FD in the library.
    return (status == files->Ok, text); # A failed read never becomes successful empty text.
}


# Write one acknowledged snapshot completely; return success only after all bytes.
# path/text are values; FD closes on return. This is checked overwrite, not atomic replacement.
writesource(path: string, text: string): int
{
    return files->write(sys, path, text) == files->Ok; # The library validates before creating or overwriting.
}


# Publish one clipboard edit through the real document owner, never the renderer.
# Range/text are copied values at the captured revision; returns the acknowledged source snapshot.
clipboardedit(document: chan of Idecore->Request, revision, first, last: int, text: string): Idecore->Reply
{
    answers := chan of Idecore->Reply;
    document <-= Idecore->Request(1, core->Replace, revision, first, last, text, "", answers);
    return <-answers;
}


# Preserve the visible last operation result across modifier/key-release/focus observations.
# Replies and event are copied/borrowed values; a new source revision always supplies its own status.
inputreply(previous, current: Idecore->Reply, event: ref Uibridge->Message): Idecore->Reply
{
    if (previous.revision == current.revision && (event.kind == Uibridge->Focus
        || (event.kind == Uibridge->Key && (!event.flags || event.code == 16 || event.code == 17)))) {
        current.status = previous.status;
    }
    return current;
}


# Find the next literal match and apply only its acknowledged Unicode selection.
# document/state are exclusively coordinated here; query is a value. Returns the real owner reply.
findnext(document: chan of Idecore->Request, state: ref Ideinput->State, policy: Ideinput,
         query: string): Idecore->Reply
{
    reply := search->next(core, document, state.revision, state.caret, query); # Search is read-only at the owner.
    if (reply.status == core->Ok) { # Select only the matched range from this acknowledged revision.
        if (!policy->select(state, reply, reply.offset, reply.offset + len query)) { # Refuse stale range publication.
            reply.status = core->Conflict; # Source/history remain unchanged.
        }
    }
    return reply; # Status NotFound and all errors stay visible to the caller.
}


# Parse a Go To field as positive decimal line or line:column, without integer overflow.
# text is borrowed; returns (0, 0) for any invalid syntax/range. No source-language parsing occurs.
locationfield(text: string): (int, int)
{
    line := 0;
    column := 0;
    separated := 0;
    digits := 0;
    for (index := 0; index < len text; index++) {
        character := text[index];
        if (character == ':' && !separated && digits > 0 && line > 0) {
            separated = 1;
            digits = 0;
        } else if (character >= '0' && character <= '9') {
            value := line;
            if (separated) {
                value = column;
            }
            digit := character - '0';
            if (value > (2147483647 - digit) / 10) {
                return (0, 0);
            }
            value = value * 10 + digit;
            if (separated) {
                column = value;
            } else {
                line = value;
            }
            digits++;
        } else {
            return (0, 0);
        }
    }
    if (digits == 0 || line < 1 || (separated && column < 1)) {
        return (0, 0);
    }
    if (!separated) {
        column = 1;
    }
    return (line, column);
}


# Move only the Dis-owned selection to a checked one-based logical source location.
# reply is the current acknowledgement; returns its copy with explicit status, without history/I/O.
gotolocation(policy: Ideinput, state: ref Ideinput->State, reply: Idecore->Reply,
             line, column: int): Idecore->Reply
{
    position := policy->locate(reply.text, line, column);
    if (position < 0) {
        reply.status = core->RangeError;
    } else if (!policy->select(state, reply, position, position)) {
        reply.status = core->Conflict;
    } else {
        reply.status = core->Ok;
    }
    return reply;
}


# Translate file-service statuses to the document/UI status namespace.
# status is copied; EOF is successful presentation, other failures are Invalid.
browserstatus(status: int): int
{
    if (status == directoryfiles->Ok || status == directoryfiles->End) {
        return core->Ok;
    }
    return core->Invalid;
}


# Stop the private directory owner before native channel borrowing ends.
# browser is exclusive to this controller; all three clean/forced close paths use this boundary.
stopbrowser(browser: ref Idebrowser->State)
{
    if (browsing->shutdown(directoryfiles, browser) != directoryfiles->Ok) {
        raise "fail: graphical browser shutdown";
    }
}


# Navigate or load the selected entry using the same dirty/source/history policy as Open.
# Inputs are borrowed until each channel reply. Return copied document reply, admitted
# destination and success flag; no destination is adopted before a validated load.
choosebrowser(browser: ref Idebrowser->State, document: chan of Idecore->Request,
              policy: Ideinput, state: ref Ideinput->State, current: Idecore->Reply): (Idecore->Reply, string, int)
{
    (kind, target) := browsing->choose(directoryfiles, browser);
    if (kind == browsing->PickDirectory) {
        current.status = core->Ok;
        return (current, "", 0);
    }
    if (kind != browsing->PickFile) {
        current.status = core->Invalid;
        return (current, "", 0);
    }
    if (current.revision != current.saved) {
        current.status = core->Conflict;
        return (current, "", 0);
    }
    (loaded, source) := readsource(target);
    if (!loaded) {
        current.status = core->Invalid;
        return (current, "", 0);
    }
    # The loaded source is admitted before retiring the old clean document; adopt no path on conflict.
    current = call(document, core->Close, current.revision, "");
    if (current.status != core->Ok) {
        return (current, "", 0);
    }
    current = call(document, core->Open, current.revision, source);
    if (current.status != core->Ok) {
        return (current, "", 0);
    }
    policy->observe(state, current);
    if (browsing->clear(directoryfiles, browser) != directoryfiles->Ok) {
        raise "fail: graphical browser cursor retirement";
    }
    return (current, target, 1);
}


# Infer the bounded compiler language from an explicit lower-case source suffix.
# path is copied configuration; zero means unsupported/unnamed and never guesses.
compilerlanguage(path: string): int
{
    endings := array[] of {".as1", ".as2", ".as3", ".as", ".b"};
    languages := array[] of {Fircompiler->AS1, Fircompiler->AS2, Fircompiler->AS3,
                            Fircompiler->AS2, Fircompiler->Limbo};
    for (index := 0; index < len endings; index++) {
        ending := endings[index];
        if (len path >= len ending && path[len path - len ending:] == ending) {
            return languages[index];
        }
    }
    return 0;
}


# Retire the optional build panel before the borrowed editor document. nil means
# the compiler capability was absent. Return only after terminal acknowledgement.
stopbuild(panel: Idebuildpanel, endpoint: chan of Idebuildpanel->Request)
{
    if (endpoint != nil) {
        answers := chan of Idebuildpanel->Reply;
        endpoint <-= Idebuildpanel->Request(1, panel->Shutdown, 0, 0, 0, 0, answers);
        stopped := <-answers;
        if (stopped.status != panel->Ok) {
            raise "fail: graphical compiler panel shutdown";
        }
    }
}


# Own graphical editor policy with no Draw/Tk/AS-side document mutation.
# No parameters; channels remain rooted on this stack through the final reply.
init()
{
    sys = load Sys Sys->PATH;
    directoryfiles = load Idefiles Idefiles->PATH;
    browsing = load Idebrowser Idebrowser->PATH; # Link the actual native file services.
    core = load Idecore Idecore->PATH; # Link the document/history module.
    codec = load Uibridge Uibridge->PATH; # Link the common channel codec.
    files = load Idefileops Idefileops->PATH; # Link the shared checked file operations.
    search = load Idesearch Idesearch->PATH; # Link renderer-independent document actions.
    bridge = load Idebridge Idebridge->PATH; # Link the existing snapshot encoder.
    policy := load Ideinput Ideinput->PATH; # Link Dis-owned input policy.
    viewport := load Ideviewport Ideviewport->PATH; # Link the bounded logical-line viewport.
    fields := load Idefield Idefield->PATH; # Keep modal field editing in reusable Dis logic.
    clipboard := load Ideclipboard Ideclipboard->PATH; # Load reusable bounded clipboard value staging.
    host := load Idehost Idehost->PATH; # Link only the native channel registration boundary.
    if (directoryfiles == nil || browsing == nil || sys == nil || core == nil || codec == nil || bridge == nil || policy == nil || host == nil || viewport == nil || files == nil || search == nil || clipboard == nil || fields == nil) { # Require all real modules.
        raise "fail: graphical editor modules"; # Stop visibly on an unsupported host.
    }
    browser := browsing->create(directoryfiles, sys);
    if (browser == nil) {
        raise "fail: graphical browser owner";
    }
    input := chan of array of byte; # The native owner offers copied events here.
    output := chan of array of byte; # The native owner receives Dis-owned snapshots here.
    document := core->start(); # Spawn the existing separate document owner.
    buildpanel := load Idebuildpanel Idebuildpanel->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    compiler := load Idecompiler Idecompiler->PATH;
    builds: chan of Idebuildpanel->Request;
    if (buildpanel != nil) {
        builds = buildpanel->start(core, document, workspace, compiler);
    } # Missing native compilation is an explicit optional capability.
    state := policy->create(); # Keep cursor/control/history policy in Dis.
    path := host->path(); # Copy the explicit host-selected destination.
    text := ""; # A new document starts empty.
    if (path != "" && !host->fresh()) { # Loading is explicit and may fail without creating a file.
        (loaded, initial) := readsource(path); # Read only through Sys.
        if (!loaded) { # Initial load must not silently discard an existing file.
            raise "fail: graphical editor initial source"; # Native process reports the error.
        }
        text = initial; # Publish only validated UTF-8.
    }
    reply := call(document, core->Open, 0, text);
    values := load Idevalue Idevalue->PATH;
    projections := load Idedocprojection Idedocprojection->PATH;
    if (values == nil || projections == nil) {
        raise "fail: graphical document projection modules";
    }
    inspector := projections->start(core, values, document, 1, "graphical-document");
    if (inspector == nil) {
        raise "fail: graphical document projection owner";
    } # Establish the first saved revision.
    policy->observe(state, reply); # Synchronize the Dis selection with that revision.
    if (host->attach(input, output) != 0) { # Borrow these live stack-rooted ports.
        raise "fail: graphical editor channel registration"; # Never continue without a receiver.
    }
    window := viewport->create(); # The Dis controller exclusively owns its viewport.
    width := 960; # Initial canvas pixel width, subsequently admitted through Resize.
    height := 640; # Initial canvas pixel height; source policy never owns native storage.
    numbers := host->linenumbers() != 0; # Configuration is copied once; visibility policy remains Dis-owned.
    closing := 0; # Save-then-close is admitted only by explicit dirty-close confirmation.
    fontsize := 11; # Default size belongs to Dis policy, not native document logic.
    window.rows = host->rows(); # Admit actual font geometry for the initial viewport.
    state.page = window.rows; # Page navigation and visible logical lines use the same units.
    reveal := 1; # Initial presentation follows the acknowledged caret.
    ticket := 0; # Monotonic view identity also distinguishes scrolling within one revision.
    mode := 0; # 0 document, 1 Find, 2 Open, 3 Save As, 4 needle, 5 replacement, 6 Go To, 7 readonly inspector.
    inputfield := fields->create(); # [GC:] Exclusive single-line field text/selection root, never a native editor buffer.
    query := ""; # Last literal query, reused by F3 and replacement.
    dragging := 0; # Only an admitted pointer begin starts a Dis selection gesture.
    fielddrag := 0; # Field anchor policy belongs to Dis, independent of document dragging.
    transfer_mode := 0; # Zero targets the source; nonzero captures the exact modal operation.
    transfer_stamp := 0; # A field edit/selection change retires a clipboard's captured generation.
    transfer := clipboard->create(); # [GC:] Exclusive pending transaction/staging root until reset or shutdown.
    buildmessage := ""; # Plain copied compiler status, bounded before rendering.
    buildvisible := 0; # Inspector mode can show compiler output or document metadata.
    for (;;) { # Serialize every input, file operation and document acknowledgement.
        if (closing && reply.revision == reply.saved) {
            stopbrowser(browser);
            releaseprojection(inspector);
            stopbuild(buildpanel, builds);
            call(document, core->Shutdown, reply.revision, "");
            host->detach();
            return;
        }
        if (transfer.action == clipboard->Paste && transfer.total >= 0 && transfer.used < transfer.total) {
            credit := ref Uibridge->Message(codec->View, 0, host->ClipboardContinue, transfer.revision,
                                           transfer.used, transfer.total, codec->hexword(transfer.ticket));
            (credited, creditwire) := codec->encode(credit);
            if (!credited) {
                raise "fail: graphical editor clipboard credit";
            }
            output <-= creditwire; # No unchanged document retransmission between private paste fragments.
        } else {
            if (ticket == 2147483647) { # Never wrap a view identity onto a stale input.
                raise "fail: graphical editor view ticket exhausted"; # Stop with explicit exhaustion.
            }
            canvas := ref Uibridge->Message(codec->View, 0, host->Canvas, reply.revision, width, height, "");
            (canvasok, canvaswire) := codec->encode(canvas); # Publish only copied pixel dimensions.
            if (!canvasok) {
                raise "fail: graphical editor canvas";
            }
            output <-= canvaswire; # Admit native allocation before its matching font/row geometry.
            style := ref Uibridge->Message(codec->View, 0, host->Style, reply.revision, fontsize, window.rows, ""); # Publish Dis-owned font/row policy.
            (styleok, stylewire) := codec->encode(style); # Style is a value, never a TextField pointer.
            if (!styleok) { # Invalid style must fail visibly before the following snapshot.
                raise "fail: graphical editor style"; # Never continue with mismatched hit geometry.
            }
            output <-= stylewire;
            gutter := ref Uibridge->Message(codec->View, 0, host->Gutter, reply.revision, numbers, 0, "");
            (gutterok, gutterwire) := codec->encode(gutter);
            if (!gutterok) {
                raise "fail: graphical editor gutter";
            }
            output <-= gutterwire; # Native geometry is committed before the matching viewport/snapshot.
            paneltext := inputfield.text; # Active input fields are Dis-owned values.
            if (mode == 0) { # An inactive panel reports the selected document path.
                paneltext = path; # Save As only changes this path after successful output.
                if (buildmessage != "") {
                    paneltext += " | " + buildmessage;
                }
                if (len paneltext > 200) { # Bound the label without changing the actual file destination.
                    paneltext = paneltext[:200]; # Scalar slicing cannot split UTF-8.
                }
            }
            if (mode == 8) {
                paneltext = "S: save and close; D: discard; Esc: cancel";
            }
            if (mode == browsing->Panel) {
                paneltext = browsing->summary(sys, browser);
            }
            if (mode == 7) {
                paneltext = buildmessage;
                if (!buildvisible) {
                    paneltext = inspectdocument(inspector);
                } # Only channel-copied fields reach the native renderer.
            }
            selected := inputfield.caret != inputfield.anchor
                && ((inputfield.anchor == 0 && inputfield.caret == len inputfield.text)
                    || (inputfield.caret == 0 && inputfield.anchor == len inputfield.text));
            if (mode == browsing->Panel) {
                selected = 0;
            }
            panel := ref Uibridge->Message(codec->View, 0, host->Panel, reply.revision, mode, selected, paneltext); # Publish prompt state by channel.
            (panelok, panelwire) := codec->encode(panel); # Validate the counted Unicode field.
            if (!panelok) { # Invalid UI state must fail visibly, not mutate native widgets directly.
                raise "fail: graphical editor panel"; # No successful dummy panel response is substituted.
            }
            output <-= panelwire; # Panel and viewport are committed together at the final selection record.
            fieldcaret := 0;
            fieldanchor := 0;
            if (mode != 0 && mode != browsing->Panel) {
                fieldcaret = inputfield.caret;
                fieldanchor = inputfield.anchor;
            }
            if (mode == browsing->Panel) {
                (fieldanchor, fieldcaret) = browsing->selection(sys, browser);
            }
            fieldview := ref Uibridge->Message(codec->View, 0, host->PanelSelection, reply.revision,
                                              fieldcaret, fieldanchor, "");
            (fieldok, fieldwire) := codec->encode(fieldview);
            if (!fieldok) {
                raise "fail: graphical editor field selection";
            }
            output <-= fieldwire; # Copied scalar field positions, independent of document selection.

            ticket++; # Give every published view a distinct scalar identity.
            (first, last, top) := viewport->window(window, reply.text, state.caret, reveal); # Bound the display window.
            metadata := codec->hexword(top) + codec->hexword(len reply.text) + codec->hexword(ticket); # No heap addresses.
            placement := ref Uibridge->Message(codec->View, 0, host->Viewport, reply.revision, first, last, metadata); # Dis owns visible bounds.
            (placed, geometry) := codec->encode(placement); # Validate the copied viewport record.
            if (!placed) { # Reject malformed metadata before any snapshot fragment.
                raise "fail: graphical editor viewport"; # Never fabricate a valid view.
            }
            output <-= geometry; # Publish viewport identity before its document fragments.
            if (!bridge->sendreply(codec, output, reply, 1)) { # Send a complete Unicode-aligned snapshot.
                raise "fail: graphical editor snapshot"; # A framing failure is terminal.
            }
            view := ref Uibridge->Message(codec->View, 0, 65536, reply.revision, state.caret, state.anchor, ""); # Selection is Dis authority.
            (encoded, selection) := codec->encode(view); # Encode only copied scalar coordinates.
            if (!encoded) { # Refuse invalid coordinates/format before sending.
                raise "fail: graphical editor selection"; # Surface the protocol failure.
            }
            output <-= selection; # Complete the view before receiving another event.
        }
        frame := <-input; # Block this process while native rendering/events continue.
        (valid, event) := codec->decode(frame); # Reject malformed transport input.
        if (!valid) { # Do not execute unvalidated event fields.
            raise "fail: graphical editor input"; # The native host reports failure.
        }
        reveal = 0; # Wheel/modifier events must not snap the viewport back to the caret.
        if (event.kind == codec->Request && event.code == host->ClipboardAck) {
            identity := array of byte event.text;
            (known, seen) := codec->parseword(identity, 0);
            if (transfer.action == clipboard->None || !known || len identity != 8 || seen != transfer.ticket
                || event.flags != transfer.revision || reply.revision != transfer.revision || transfer_mode != mode
                || (transfer_mode != 0 && transfer_stamp != inputfield.stamp)) {
                reply.status = core->Conflict;
            } else if (event.second != 0 || (event.first != core->Ok && event.first != core->TooLarge && event.first != core->Invalid)
                       || (transfer.action == clipboard->Paste && event.first == core->Ok)) {
                reply.status = core->Invalid;
            } else if (event.first != core->Ok) {
                reply.status = event.first; # Native errors never authorize source deletion.
            } else if (transfer.action == clipboard->Cut && transfer_mode != 0) {
                reply.status = fields->insert(inputfield, ""); # No document edit or Undo entry for field cut.
            } else if (transfer.action == clipboard->Cut) {
                reply = clipboardedit(document, transfer.revision, transfer.first, transfer.last, "");
                if (reply.status == core->Ok) {
                    position := transfer.first;
                    policy->observe(state, reply);
                    policy->select(state, reply, position, position);
                    reveal = 1;
                }
            } else {
                reply.status = core->Ok; # Copy changes neither selection nor source/history/saved state.
            }
            clipboard->reset(transfer);
            continue;
        }
        if (event.kind == codec->Request && event.code == host->ClipboardData) {
            first := transfer.first; # Retain value coordinates before a complete fragment releases staging.
            last := transfer.last;
            captured := transfer.revision;
            if (captured != reply.revision || transfer_mode != mode
                || (transfer_mode != 0 && transfer_stamp != inputfield.stamp)) {
                clipboard->reset(transfer);
                reply.status = core->Conflict;
            } else {
                (status, ready, text) := clipboard->chunk(codec, transfer, event);
                reply.status = status;
                if (ready && text != "" && transfer_mode != 0) {
                    reply.status = fields->insert(inputfield, text); # Complete single-line admission before any field mutation.
                } else if (ready && text != "") { # Empty clipboard is a no-op, including a nonempty selection.
                    reply = clipboardedit(document, captured, first, last, text);
                    if (reply.status == core->Ok) {
                        position := first + len text;
                        policy->observe(state, reply);
                        policy->select(state, reply, position, position);
                        reveal = 1;
                    }
                }
            }
            continue; # No partial prefix is ever submitted as an edit or Undo entry.
        }
        if (transfer.action != clipboard->None) {
            clipboard->reset(transfer); # Unrelated input retires the staged range and all private bytes.
        }
        if (event.kind == codec->Request && event.code == host->Quit) { # Close works even inside a modal field.
            if (reply.revision == reply.saved || event.flags == 1) {
                stopbrowser(browser);
                releaseprojection(inspector);
                stopbuild(buildpanel, builds);
                call(document, core->Shutdown, reply.revision, "");
                host->detach(); # Retire borrowed channel roots before the controller stack exits.
                return;
            }
            reply.status = core->Conflict;
            mode = 8; # Closing an unsaved window must visibly ask rather than silently do nothing.
            closing = 0;
            inputfield = fields->create();
            fielddrag = 0;
            continue;
        }
        if (event.kind == codec->Request && event.code == host->Resize) {
            metadata := array of byte event.text; # Exactly two canonical words, never a pointer/struct.
            (known, seen) := codec->parseword(metadata, 0);
            (measured, rows) := codec->parseword(metadata, 8);
            if (!known || !measured || len metadata != 16 || seen != ticket || event.flags != reply.revision) {
                reply.status = core->Conflict; # Retain canvas and source when geometry is stale/malformed.
            } else if (event.first < 240 || event.first > 2048 || event.second < 160 || event.second > 2048
                       || rows < 1 || rows > 128) {
                reply.status = core->RangeError; # Never allocate an unbounded native surface.
            } else {
                width = event.first;
                height = event.second;
                window.rows = rows; # The native metric is a bounded proposal admitted on Dis.
                state.page = rows;
                dragging = 0; # The old pixel gesture cannot continue across a changed canvas.
                fielddrag = 0;
                reply.status = core->Ok; # Document/history/saved state do not change.
                reveal = 1;
            }
            continue; # Resize works inside a prompt without becoming source input.
        }
        if (event.kind == codec->Request && event.code == host->Zoom) { # Wheel/pinch changes are still requests to Dis.
            expected := array of byte event.text; # Bind geometry to the exact completed view.
            (known, seen) := codec->parseword(expected, 0); # Ticket changes even without a content edit.
            if (!known || len expected != 8 || seen != ticket || event.flags != reply.revision) { # Reject stale gesture geometry.
                reply.status = core->Conflict; # Text, history and current size remain unchanged.
            } else if ((event.first != -1 && event.first != 1) || event.second < 1 || event.second > 128) { # Bound delta and measured rows.
                reply.status = core->RangeError; # Do not accept unbounded native geometry proposals.
            } else { # Apply explicit zoom policy on Dis.
                candidate := fontsize + event.first; # One bounded wheel/pinch step per admitted event.
                if (candidate < 6) { # Keep the minimum usable size explicit.
                    candidate = 6; # Clamp without wrapping a view or font identity.
                }
                if (candidate > 48) { # Avoid unbounded font/layout allocation.
                    candidate = 48; # A gesture at the maximum remains a no-op content-wise.
                }
                fontsize = candidate; # The renderer receives this size only through Style.
                fielddrag = 0; # Old field pixel gestures cannot continue through a changed face size.
                window.rows = event.second; # Native metrics are admitted as copied bounded values.
                state.page = window.rows; # Keep page movement consistent after scaling.
                reply.status = core->Ok; # Zoom never changes content revision or saved state.
                reveal = 1; # Reveal the current caret in the resized logical viewport.
            }
            continue; # A zoom event is never also a text selection or ordinary scroll.
        }
        if (event.kind == codec->Key && event.flags != 0 && state.control && event.code == 76) {
            numbers = !numbers; # Ctrl+L hides/shows the gutter without a source edit or Undo entry.
            dragging = 0;
            fielddrag = 0;
            continue;
        }
        # A completed-view ticket binds selection actions even when document revision is unchanged.
        if (event.kind == codec->Request && event.code >= host->Browse && event.code <= host->BrowseClose) {
            if (event.flags != reply.revision || event.second != ticket) {
                reply.status = core->Conflict;
            } else if (event.code != host->BrowseMove && event.first != 0 ||
                       event.code != host->Browse && event.text != "") {
                reply.status = core->Invalid;
            } else if (event.code == host->Browse) {
                reply.status = browserstatus(browsing->open(directoryfiles, browser, event.text));
                if (reply.status == directoryfiles->Ok) {
                    mode = browsing->Panel;
                    inputfield = fields->create();
                    dragging = 0;
                    fielddrag = 0;
                }
            } else if (mode != browsing->Panel) {
                reply.status = core->Conflict;
            } else if (event.code == host->BrowseMove) {
                reply.status = browserstatus(browsing->move(browser, event.first));
            } else if (event.code == host->BrowseNext) {
                status := browsing->advance(directoryfiles, browser);
                reply.status = core->Ok;
                if (status != directoryfiles->Ok && status != directoryfiles->End) {
                    reply.status = core->Invalid;
                }
            } else if (event.code == host->BrowseClose) {
                reply.status = browserstatus(browsing->clear(directoryfiles, browser));
                mode = 0;
            } else {
                (updated, destination, opened) := choosebrowser(browser, document, policy, state, reply);
                reply = updated;
                if (opened) {
                    path = destination;
                    mode = 0;
                    reveal = 1;
                }
            }
            continue;
        }
        # Read-only browsing consumes source text/clipboard/pointer events before field or edit dispatch.
        if (mode == browsing->Panel) {
            if (event.kind == codec->Focus || event.kind == codec->Key && (event.code == 16 || event.code == 17)) {
                (admitted, observed) := policy->apply(core, document, state, event);
                if (!admitted) {
                    raise "fail: browser modifier event";
                }
                reply = inputreply(reply, observed.document, event);
            } else if (event.kind == codec->Request && event.code == host->FieldPlace) {
                identity := array of byte event.text;
                (known, seen) := codec->parseword(identity, 0);
                if (!known || len identity != 8 || seen != ticket || event.flags != reply.revision) {
                    reply.status = core->Conflict;
                } else if (event.second < 1 || event.second > 3) {
                    reply.status = core->RangeError;
                } else if (event.second == 1) {
                    action := browsing->hit(sys, browser, event.first);
                    if (action >= 0) {
                        browser.selected = action;
                        reply.status = core->Ok;
                    } else if (action == -2) {
                        reply.status = browserstatus(browsing->parent(directoryfiles, browser));
                    } else if (action == -3) {
                        reply.status = browserstatus(browsing->advance(directoryfiles, browser));
                    } else if (action == -4) {
                        reply.status = browserstatus(browsing->open(directoryfiles, browser, browser.path));
                    } else if (action == -5) {
                        mode = browsing->PathPanel;
                        inputfield = fields->create();
                        reply.status = fields->set(inputfield, browser.path, 1);
                    } else if (action == -6) {
                        reply.status = browserstatus(browsing->clear(directoryfiles, browser));
                        mode = 0;
                    } else if (action == -7) {
                        (updated, destination, opened) := choosebrowser(browser, document, policy, state, reply);
                        reply = updated;
                        if (opened) {
                            path = destination;
                            mode = 0;
                            reveal = 1;
                        }
                    }
                }
            } else if (event.kind == codec->Key && event.flags != 0) {
                if (event.code == 8) {
                    reply.status = browserstatus(browsing->parent(directoryfiles, browser));
                } else if (event.code == 27) {
                    reply.status = browserstatus(browsing->clear(directoryfiles, browser));
                    mode = 0;
                    inputfield = fields->create();
                } else if (event.code == 66) {
                    mode = browsing->PathPanel;
                    inputfield = fields->create();
                    reply.status = fields->set(inputfield, browser.path, 1);
                } else if (event.code == 38 || event.code == 40) {
                    delta := 1;
                    if (event.code == 38) {
                        delta = -1;
                    }
                    reply.status = browserstatus(browsing->move(browser, delta));
                } else if (event.code == 34) {
                    status := browsing->advance(directoryfiles, browser);
                    reply.status = core->Ok;
                    if (status != directoryfiles->Ok && status != directoryfiles->End) {
                        reply.status = core->Invalid;
                    }
                } else if (event.code == 82) {
                    reply.status = browserstatus(browsing->open(directoryfiles, browser, browser.path));
                } else if (event.code == 13) {
                    (updated, destination, opened) := choosebrowser(browser, document, policy, state, reply);
                    reply = updated;
                    if (opened) {
                        path = destination;
                        mode = 0;
                        reveal = 1;
                    }
                }
            }
            continue; # Browsing never forwards text, paste, delete or pointer gestures to source input.
        }
        if ((mode == 0 || mode == 7) && event.kind == codec->Key && event.flags == 1
            && (event.code == 118 || event.code == 119)) { # F7 Build, F8 source diagnostic.
            buildvisible = 1;
            buildmessage = "Compiler unavailable: enable Fircompiler";
            if (builds != nil) {
                language := compilerlanguage(path);
                if (event.code == 118 && language == 0) {
                    buildmessage = "Save As .b, .as, .as1, .as2 or .as3 before Build";
                } else {
                    operation := buildpanel->Build;
                    target := Fircompiler->Swf;
                    if (language == Fircompiler->Limbo) {
                        target = Fircompiler->Dis;
                    }
                    if (event.code == 119) {
                        operation = buildpanel->Diagnostic;
                    }
                    answers := chan of Idebuildpanel->Reply;
                    builds <-= Idebuildpanel->Request(1, operation, reply.revision,
                                                      language, target, 1, answers);
                    built := <-answers;
                    buildmessage = "Build status=" + string built.status + " stage=" + string built.compiler;
                    if (built.diagnostic != "") {
                        buildmessage += ": " + built.diagnostic;
                    }
                    if (event.code == 119 && built.status == buildpanel->Ok) {
                        reply = built.document;
                        if (!policy->select(state, reply, reply.offset, reply.selection_end)) {
                            raise "fail: graphical compiler diagnostic revision";
                        }
                        mode = 0;
                        reveal = 1;
                        continue;
                    }
                }
            }
            # Every Unicode scalar needs at most four UTF-8 bytes on this channel.
            if (len buildmessage > codec->MaxText / 4) {
                buildmessage = buildmessage[:codec->MaxText / 4];
            }
            mode = 7; # Read-only output; Esc returns to source without editing it.
            dragging = 0;
            fielddrag = 0;
            inputfield = fields->create();
            continue;
        }
        if (mode == 7 || mode == 8) { # Read-only inspector/confirmation never forward text or paste to the source.
            if (event.kind == codec->Key && event.flags != 0 && event.code == 27) {
                mode = 0;
                closing = 0;
                inputfield = fields->create();
                fielddrag = 0;
                reply.status = core->Ok;
            } else if (mode == 7 && event.kind == codec->Key && event.flags != 0
                       && event.code == 73 && state.control) {
                mode = 0;
                inputfield = fields->create();
                reply.status = core->Ok;
            } else if (mode == 8 && event.kind == codec->Key && event.flags != 0 && event.code == 68) {
                stopbrowser(browser);
                releaseprojection(inspector); # Explicit D confirms discard; the close button itself never does.
                stopbuild(buildpanel, builds);
                call(document, core->Shutdown, reply.revision, "");
                host->detach();
                return;
            } else if (mode == 8 && event.kind == codec->Key && event.flags != 0 && event.code == 83) {
                closing = 1;
                if (path == "") {
                    mode = 3; # Save As must finish successfully before the next loop can close.
                    inputfield = fields->create();
                } else if (writesource(path, reply.text)) {
                    reply = call(document, core->MarkSaved, reply.revision, "");
                    mode = 0;
                } else {
                    reply.status = core->Invalid; # Failed output stays in the visible confirmation.
                    closing = 0;
                }
            } else if (event.kind == codec->Focus || event.kind == codec->Key && (event.code == 16 || event.code == 17)) {
                (admitted, observed) := policy->apply(core, document, state, event);
                if (!admitted) {
                    raise "fail: readonly panel modifier event";
                }
                reply = inputreply(reply, observed.document, event);
            }
            continue;
        }
        if (event.kind == codec->Request && event.code == host->FieldPlace) {
            identity := array of byte event.text;
            (known, seen) := codec->parseword(identity, 0);
            if (mode == 0 || !known || len identity != 8 || seen != ticket || event.flags != reply.revision) {
                reply.status = core->Conflict;
            } else if (event.second < 1 || event.second > 3 || event.first > len inputfield.text) {
                reply.status = core->RangeError;
            } else if (event.first < 0) {
                if (event.second == 3) {
                    fielddrag = 0; # Mouse release outside the footer cannot leave an active gesture.
                }
            } else if (event.second == 1 || fielddrag) {
                anchor := inputfield.anchor;
                if (event.second == 1 && !state.shift) {
                    anchor = event.first;
                }
                reply.status = fields->select(inputfield, anchor, event.first);
                fielddrag = reply.status == core->Ok && event.second != 3;
            }
            continue; # Field placement cannot move source selection or change document history.
        }
        if (event.kind == codec->Key && event.flags != 0 && state.control) {
            if (event.code == 73) { # Ctrl+I opens a real document projection, never a native model shortcut.
                mode = 7;
                buildvisible = 0;
                inputfield = fields->create();
                dragging = 0;
                fielddrag = 0;
                reply.status = core->Ok;
                continue;
            }
            if (event.code == 67 || event.code == 88 || event.code == 86) { # Ctrl+C/X/V capture the active Dis source or field domain.
                first := state.caret;
                last := state.anchor;
                if (mode != 0) {
                    first = inputfield.caret;
                    last = inputfield.anchor;
                }
                if (first > last) {
                    (first, last) = (last, first);
                }
                action := clipboard->Copy;
                code := host->ClipboardWrite;
                if (mode != 0) {
                    code = host->ClipboardFieldWrite;
                }
                if (event.code == 88) {
                    action = clipboard->Cut;
                } else if (event.code == 86) {
                    action = clipboard->Paste;
                    code = host->ClipboardRead;
                }
                if (first == last && action != clipboard->Paste) {
                    reply.status = core->Ok; # No selection leaves the native clipboard and history intact.
                    continue;
                }
                if (!clipboard->begin(transfer, action, ticket, reply.revision, first, last)) {
                    raise "fail: graphical editor clipboard admission";
                }
                transfer_mode = mode;
                transfer_stamp = inputfield.stamp;
                fielddrag = 0;
                low := first;
                high := last;
                if (action == clipboard->Paste) {
                    low = 0;
                    high = 0;
                }
                command := ref Uibridge->Message(codec->View, 0, code, reply.revision, low, high, codec->hexword(ticket));
                (accepted, wire) := codec->encode(command);
                if (!accepted) {
                    raise "fail: graphical editor clipboard request";
                }
                output <-= wire; # Native access returns copied acknowledgement/data events, never an edit.
                dragging = 0;
                reply.status = core->Ok;
                continue;
            }
        }
        if (mode != 0) { # Modal field policy executes entirely on Dis.
            if (event.kind == codec->Focus || (event.kind == codec->Key && (event.code == 16 || event.code == 17))) { # Keep modifier/focus state accurate.
                (admitted, observed) := policy->apply(core, document, state, event); # Reuse the production modifier controller.
                if (!admitted) { # Never use malformed input to alter field policy.
                    raise "fail: graphical editor prompt modifier"; # Report a protocol failure.
                }
                reply = inputreply(reply, observed.document, event); # Modifier release cannot erase a field error.
                if (event.kind == codec->Focus && event.flags == 0) { # Focus loss retires any gesture.
                    dragging = 0; # Returning focus cannot resume a hidden drag.
                    fielddrag = 0;
                }
            } else if (event.kind == codec->Text || (event.kind == codec->Key && event.flags != 0)) {
                if (event.kind == codec->Key && event.code == 27) {
                    if (mode == browsing->PathPanel && browser.cursor != big 0) {
                        mode = browsing->Panel;
                    } else {
                        mode = 0;
                    }
                    closing = 0;
                    fielddrag = 0;
                    inputfield = fields->create(); # Retire this field session without changing source or path.
                    reply.status = core->Ok;
                } else if (event.kind == codec->Key && event.code == 13) { # Enter executes an explicitly accepted field operation.
                    fielddrag = 0; # A command cannot leave an old field drag active across a mode change.
                    if (mode == browsing->PathPanel) {
                        reply.status = browserstatus(browsing->open(directoryfiles, browser, inputfield.text));
                        if (reply.status == directoryfiles->Ok) {
                            mode = browsing->Panel;
                            inputfield = fields->create();
                        }
                    } else if (mode == 6) { # Go To uses bounded one-based source coordinates.
                        (line, column) := locationfield(inputfield.text); # Reject syntax/overflow without touching selection.
                        reply = gotolocation(policy, state, reply, line, column);
                        if (reply.status == core->Ok) {
                            mode = 0; # Invalid/absent locations keep the editable field open.
                            reveal = 1; # Reveal the admitted scalar caret through the normal viewport.
                        }
                    } else if (mode == 1) { # Literal Find selects the real acknowledged match.
                        query = inputfield.text; # Retain the user's query for F3.
                        reply = findnext(document, state, policy, query); # Search through the document channel.
                        mode = 0; # Errors/NotFound are visible in the normal status view.
                        reveal = 1; # Reveal a successful match's caret.
                    } else if (mode == 4) { # Replacement is a two-field operation, without delimiter ambiguity.
                        if (inputfield.text == "") { # An empty needle is invalid before any replacement stage.
                            reply.status = core->Invalid; # Keep this field open for correction.
                        } else { # Preserve the exact literal needle.
                            query = inputfield.text; # Unicode/non-BMP text stays unchanged.
                            inputfield = fields->create(); # Begin a separately counted replacement field.
                            mode = 5; # The next Enter confirms replacement text, including empty text.
                        }
                    } else if (mode == 5) { # ReplaceAll remains one real document-owner operation.
                        reply = search->replace(core, document, reply.revision, query, inputfield.text); # Bounds/history are checked by Idecore.
                        policy->observe(state, reply); # Synchronize selection after an acknowledged edit.
                        mode = 0; # Publish actual match count/status with the resulting snapshot.
                        reveal = 1; # Show the post-operation cursor.
                    } else if (mode == 3) { # Save As adopts its destination only after a successful write.
                        if (writesource(inputfield.text, reply.text)) { # Preserve exact current snapshot bytes.
                            path = inputfield.text; # Subsequent Ctrl+S and reload now use this successful destination.
                            reply = call(document, core->MarkSaved, reply.revision, ""); # Acknowledge that exact revision.
                            mode = 0; # Close the field only after success.
                        } else { # A failed candidate does not replace the current path.
                            reply.status = core->Invalid; # Keep the field open and preserve dirty state.
                        }
                    } else if (mode == 2) { # Open never discards unsaved edits.
                        if (reply.revision != reply.saved) { # Check before touching the candidate file.
                            reply.status = core->Conflict; # Keep the original document and destination.
                        } else { # Read and validate before retiring the current session.
                            (loaded, source) := readsource(inputfield.text); # I/O policy stays on Dis.
                            if (loaded) { # Only validated bytes can replace the document.
                                reply = call(document, core->Close, reply.revision, ""); # Retire old history.
                                reply = call(document, core->Open, reply.revision, source); # Establish a fresh saved revision.
                                path = inputfield.text; # Adopt the successfully loaded path.
                                policy->observe(state, reply); # Reset old source positions.
                                window = viewport->create(); # Retire old scroll coordinates too.
                                window.rows = state.page; # Preserve admitted font geometry.
                                dragging = 0; # Retire old gesture state.
                                mode = 0; # Return to the new document.
                                reveal = 1; # Show its initial caret.
                            } else { # Failed UTF-8/limit/open admission changes no document state.
                                reply.status = core->Invalid; # Leave the field open for correction.
                            }
                        }
                    }
                } else {
                    (status, handled) := fields->apply(policy, inputfield, event, state.control, state.shift);
                    if (handled) {
                        reply.status = status;
                    }
                }
            }
            continue; # A field event is never also applied to the source document.
        }
        if (event.kind == codec->Key && event.flags != 0 && state.control) { # Application shortcuts remain Dis-owned.
            if (event.code == 66) { # Ctrl+B opens a directory path, never a source-edit shortcut.
                mode = browsing->PathPanel;
                inputfield = fields->create();
                prefill := browser.path;
                if (prefill == "") {
                    prefill = ".";
                }
                reply.status = fields->set(inputfield, prefill, 1);
                dragging = 0;
                fielddrag = 0;
                continue;
            }
            if (event.code == 70 || event.code == 71 || event.code == 72 || event.code == 79 || (event.code == 83 && state.shift)) { # F/H/O/Shift+S open a field.
                mode = 1; # Ctrl+F starts literal Find.
                prefill := query; # Reuse the previous literal query as editable field text.
                if (event.code == 71) { # Ctrl+G opens the source location field.
                    mode = 6; # All input and validation remain on Dis.
                    prefill = ""; # No native source position is inferred.
                }
                if (event.code == 72) { # Ctrl+H begins the needle stage of Replace All.
                    mode = 4; # Replacement text is accepted in a separate second field.
                }
                if (event.code == 79) { # Ctrl+O selects a source file.
                    mode = 2; # Open retains the current document until successful admission.
                    prefill = path; # Prefill only the editable field, not a native file dialog.
                }
                if (event.code == 83) { # Ctrl+Shift+S chooses a save destination.
                    mode = 3; # Destination adoption is conditional on successful I/O.
                    prefill = path; # Preserve the original path until that acknowledgement.
                }
                inputfield = fields->create(); # A new field session cannot reuse stale selection/generation state.
                reply.status = fields->set(inputfield, prefill, 1); # Invalid/oversize prefill leaves a safe empty field.
                dragging = 0; # A modal action retires any document drag gesture.
                fielddrag = 0;
                continue; # The shortcut must not reach document insertion/navigation.
            }
        }
        if (event.kind == codec->Key && event.flags != 0 && event.code == 114) { # F3 repeats the current literal query.
            reply = findnext(document, state, policy, query); # At most one wrap, always revision checked.
            reveal = 1; # Reveal the acknowledged match when found.
            continue; # Never interpret F3 as text or a second document operation.
        }
        if (event.kind == codec->Request && event.code == host->GoTo) { # Embedding clients use the same policy.
            if (event.flags != reply.revision) {
                reply.status = core->Conflict;
            } else if (event.text != "") {
                reply.status = core->Invalid;
            } else {
                reply = gotolocation(policy, state, reply, event.first, event.second);
                reveal = reply.status == core->Ok;
                dragging = 0; # Old pointer gestures cannot resume after an external location change.
            }
            continue; # Source/history/saved state remain unchanged.
        }
        if (event.kind == codec->Request && (event.code == host->Find || event.code == host->SaveAs || event.code == host->Open)) { # Callable actions share the same sequential controller.
            if (event.flags != reply.revision) { # External callers must identify their source snapshot.
                reply.status = core->Conflict; # A late command cannot search/save/open against another revision.
                continue; # Publish the unchanged source and path with a new view identity.
            }
            if (event.code == host->Find) { # Embedding clients can issue the same literal FindNext action.
                query = event.text; # Retain the exact counted query for F3.
                reply = findnext(document, state, policy, query); # No direct selection mutation bypasses the policy.
                reveal = 1; # Follow a successful match.
            } else if (event.code == host->SaveAs) { # An explicit path remains a copied value.
                if (writesource(event.text, reply.text)) { # Invalid/NUL paths fail before native create.
                    path = event.text; # Adopt only the successfully written destination.
                    reply = call(document, core->MarkSaved, reply.revision, ""); # Acknowledge the written snapshot.
                } else { # File errors never mark the document saved.
                    reply.status = core->Invalid; # Preserve old path and dirty state.
                }
            } else if (reply.revision != reply.saved) { # Open must not bypass the dirty guard.
                reply.status = core->Conflict; # Neither source nor destination changes.
            } else { # A clean document may be replaced only after strict source admission.
                (loaded, source) := readsource(event.text); # Same callable file library as the UI field.
                if (loaded) { # A rejected candidate never retires the current document/history.
                    reply = call(document, core->Close, reply.revision, ""); # Stop the old session.
                    reply = call(document, core->Open, reply.revision, source); # Establish the validated source.
                    path = event.text; # Adopt the loaded source path.
                    policy->observe(state, reply); # Retire stale selections and goals.
                    window = viewport->create(); # Retire old source viewport coordinates.
                    window.rows = state.page; # Preserve admitted font geometry.
                    dragging = 0; # Retire old drag state.
                    reveal = 1; # Display the new document start.
                } else { # Read failure is visible and nondestructive.
                    reply.status = core->Invalid; # Keep the original source and path.
                }
            }
            continue; # Do not also dispatch this application action as raw input.
        }
        if (event.kind == codec->Request && (event.code == host->Scroll || event.code == host->Place)) { # Admit native geometry only as a request.
            expected := array of byte event.text; # The ticket is an exact eight-byte hexadecimal word.
            (known, seen) := codec->parseword(expected, 0); # Decode without trusting a native pointer.
            if (!known || len expected != 8 || seen != ticket || event.flags != reply.revision) { # Reject old revision or viewport geometry.
                reply.status = core->Conflict; # Preserve text, selection and history on stale input.
                continue; # Publish a fresh view identity with the failure status.
            }
            if (event.code == host->Scroll) { # Scroll never edits or moves the caret.
                viewport->scroll(window, reply.text, event.first); # Clamp the logical-line delta in Dis.
                reply.status = core->Ok; # Acknowledge the bounded viewport operation.
            } else { # The renderer only proposed a source scalar from its actual glyph layout.
                position := event.first; # Coordinates remain source Unicode scalars.
                phase := event.second; # Begin/update/end are 1/2/3, not native button structs.
                if (position < 0 || position > len reply.text || phase < 1 || phase > 3) { # Validate before changing selection state.
                    reply.status = core->RangeError; # Invalid geometry changes nothing.
                } else if (phase == 1 || dragging) { # An update alone cannot invent a drag gesture.
                    anchor := state.anchor; # Preserve the original gesture or Shift anchor.
                    if (phase == 1 && !state.shift) { # Ordinary click collapses selection.
                        anchor = position; # Only Dis makes this selection decision.
                    }
                    if (!policy->select(state, reply, anchor, position)) { # Recheck current document bounds/revision.
                        reply.status = core->Conflict; # No guessed stale coordinate is applied.
                    } else { # The acknowledged selection now belongs to this controller.
                        dragging = phase != 3; # End releases the gesture after its last selection.
                        reveal = 1; # Keep an admitted pointer selection in the displayed interval.
                        reply.status = core->Ok; # Acknowledge without changing document history.
                    }
                }
            }
            continue; # The ordinary input path must not apply the same event a second time.
        }
        if (event.kind == codec->Focus && event.flags == 0) { # Losing focus ends any pending drag.
            dragging = 0; # A later update cannot extend an old gesture.
        }
        if (event.kind == codec->Text || (event.kind == codec->Key && event.flags != 0
            && event.code != 16 && event.code != 17)) { # Navigation/editing reveal their caret; releases do not.
            reveal = 1; # Viewport policy remains in Dis.
        }
        operation := event.code; # Explicit request operation or virtual-key value.
        if (event.kind == codec->Key && event.flags != 0 && state.control) { # Shortcuts remain Dis policy.
            if (event.code == 83) { # Ctrl+S selects the configured destination.
                operation = host->Save; # Save without native document authority.
            } else if (event.code == 79) { # Ctrl+O reloads that same path.
                operation = host->Reload; # Dirty-state checks follow below.
            }
        }
        if ((event.kind == codec->Request || event.kind == codec->Key) && operation == host->Save) { # Serialize save with editing.
            reply = call(document, core->Snapshot, 0, ""); # Capture the exact current revision.
            if (writesource(path, reply.text)) { # Acknowledge only actual successful output.
                reply = call(document, core->MarkSaved, reply.revision, ""); # Mark that revision saved.
            } else { # Keep the original revision/saved metadata on failure.
                reply.status = core->Invalid; # Present a visible save error.
            }
        } else if ((event.kind == codec->Request || event.kind == codec->Key) && operation == host->Reload) { # Reload through Dis I/O.
            if (reply.revision != reply.saved) { # Preserve unsaved changes.
                reply.status = core->Conflict; # Signal the refused reload.
            } else { # Only a clean document can be replaced.
                (loaded, source) := readsource(path); # Validate before changing the owner.
                if (loaded) { # Close/open establishes a fresh saved document session.
                    reply = call(document, core->Close, reply.revision, ""); # Retire old history first.
                    reply = call(document, core->Open, reply.revision, source); # Install validated text.
                    policy->observe(state, reply); # Reset selection to the new document.
                } else { # Failed reads must preserve text/history.
                    reply.status = core->Invalid; # Publish the failure.
                }
            }
        } else { # Ordinary keys/text/focus use the existing production Dis controller.
            (admitted, result) := policy->apply(core, document, state, event); # Execute through document channels.
            if (!admitted) { # Reject unknown/malformed input explicitly.
                raise "fail: graphical editor input policy"; # Native host reports the error.
            }
            reply = inputreply(reply, result.document, event); # Keep diagnostics until a real new operation.
        }
    }
}
