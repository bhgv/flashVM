implement Idedeskpane;
include "idesyntax.m";
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "ideinput.m";
include "ideviewport.m";
include "idefileops.m";
include "ideworkspace.m";
include "fircompiler.m";
include "idecompiler.m";
include "idebuildpanel.m";
include "idedeskpane.m";
include "idedeskclip.m";
include "idevector.m";


# Complete one private document exchange; borrowed endpoint stays live and no
# reply channel escapes. Returns the actual copied document snapshot/status.
call(document: chan of Idecore->Request, operation, revision: int, text: string): Idecore->Reply
{
    replies := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, revision, 0, 0, text, "", replies);
    return <-replies;
}


# Select only explicit lower-case bounded frontend suffixes; zero rejects unknown
# source languages before a native library call. path is borrowed immutable text.
language(path: string): int
{
    endings := array[] of {".b", ".as1", ".as2", ".as3", ".as"};
    values := array[] of {Fircompiler->Limbo, Fircompiler->AS1, Fircompiler->AS2, Fircompiler->AS3, Fircompiler->AS2};
    for (index := 0; index < len endings; index++) {
        ending := endings[index];
        if (len path >= len ending && path[len path - len ending:] == ending) {
            return values[index];
        }
    }
    return 0;
}


# Select the editor lexical profile independently of compiler availability.
# Untitled source defaults to Limbo; unknown suffixes keep ordinary text presentation.
syntaxlanguage(path: string): int
{
    if (path == "" || language(path) == Fircompiler->Limbo) {
        return Idesyntax->Limbo;
    }
    if (language(path) != 0) {
        return Idesyntax->ActionScript;
    }
    return 0;
}


# Rendezvous with the lazy owner, whose Update/Snapshot never waits for scanning.
# Arguments are copied values; the reply channel is retained until acknowledgement.
syntaxcall(owner: chan of Idesyntax->Command, operation: int,
           job: Idesyntax->Request): Idesyntax->Result
{
    reply := chan of Idesyntax->Result;
    owner <-= Idesyntax->Command(operation, job, reply);
    return <-reply;
}


# Admit complete session clipboard text independently of the broker caller.
# text is borrowed Unicode; return an Idecore status without changing source.
# Keep the shared byte bound and reject values native C-string clipboards cannot
# represent, so a direct library client cannot bypass ordinary UI admission.
clipstatus(text: string): int
{
    if (len text > Idedeskclip->MaxBytes) {
        return Idecore->TooLarge;
    }
    for (index := 0; index < len text; index++) {
        value := text[index];
        if (value <= 0 || value > 16r10ffff || (value >= 16rd800 && value <= 16rdfff)) {
            return Idecore->Invalid;
        }
    }
    if (len array of byte text > Idedeskclip->MaxBytes) {
        return Idecore->TooLarge;
    }
    return Idecore->Ok;
}


# Own one editor/log/help process. All dependencies and mutable document/input
# state remain on this stack. Native compilation/file I/O block the VM owner;
# separate Limbo processes do not imply parallel native parser or UI execution.
serve(kind: int, path, text: string, requests: chan of Request, sys: Sys,
      core: Idecore, policy: Ideinput, viewport: Ideviewport, files: Idefileops, panel: Idebuildpanel,
      workspace: Ideworkspace, compiler: Idecompiler, syntax: Idesyntax,
      analysis: chan of Idesyntax->Command, ready, notices: chan of int, identity: int)
{
    document: chan of Idecore->Request;
    builds: chan of Idebuildpanel->Request;
    state := policy->create();
    visible := viewport->create();
    visible.rows = 10;
    current := Idecore->Reply(0, core->Ok, 0, 0, 0, "", -1, -1, -1, -1, -1);
    if (kind == Editor) {
        document = core->start();
        current = call(document, core->Open, 0, text);
        policy->observe(state, current);
        builds = panel->start(core, document, workspace, compiler);
    }
    diagnostic := "";
    helpfirst := 0;
    pointing := 0; # Active admitted pointer sequence for this document only.
    pointanchor := 0; # Full scalar endpoint captured at pointer begin.
    submittedrevision := -1;
    submittedlanguage := 0;
    syntaxticket := 0;
    colored := Idesyntax->Result(0, 0, 0, Idesyntax->Pending, nil, 0, 0, 0);
    for (;;) {
        request: Request;
        if (analysis == nil) {
            request = <-requests;
        } else {
            alt {
            request = <-requests => ;
            <-ready =>
                colored = syntaxcall(analysis, syntax->Snapshot,
                    Idesyntax->Request(0, 0, 0, "", 0));
                if (notices != nil) {
                    alt {
                    notices <-= identity => ;
                    * => ;
                    }
                }
                continue;
            }
        }
        if (request.reply == nil) {
            continue;
        }
        if ((request.operation != Place && request.operation != Snapshot) ||
            (request.operation == Place && request.event == nil)) {
            pointing = 0;
        }
        transfer := "";
        status := core->Invalid;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Shutdown) {
                if (analysis != nil) {
                    syntaxcall(analysis, syntax->Shutdown, Idesyntax->Request(0, 0, 0, "", 0));
                    analysis = nil;
                }
                if (builds != nil) {
                    answers := chan of Idebuildpanel->Reply;
                    builds <-= Idebuildpanel->Request(1, panel->Shutdown, 0, 0, 0, 0, answers);
                    stopped := <-answers;
                    if (stopped.status != panel->Ok) {
                        raise "fail: desktop pane compiler shutdown";
                    }
                }
                if (document != nil) {
                    current = call(document, core->Shutdown, current.revision, "");
                    if (current.status != core->Ok) {
                        raise "fail: desktop pane document shutdown";
                    }
                }
                status = core->Ok;
                stop = 1;
            } else if (request.operation == CancelPointer) {
                pointing = 0;
                status = core->Ok;
            } else if (request.operation == Snapshot) {
                status = core->Ok;
            } else if (request.operation == SetText && kind == Console && len request.text <= 4096) {
                text = request.text;
                status = core->Ok;
            } else if (kind == Help && request.operation == Input && request.event != nil) {
                if (request.event.kind == Uibridge->Key && request.event.flags == 1) {
                    if (request.event.code == 34) {
                        helpfirst += 150;
                    }
                    if (request.event.code == 33) {
                        helpfirst -= 150;
                    }
                    if (helpfirst > len text - 200) {
                        helpfirst = len text - 200;
                    }
                    if (helpfirst < 0) {
                        helpfirst = 0;
                    }
                }
                status = core->Ok;
            } else if (kind == Editor) {
                if (request.operation == ExportSelection || request.operation == CutSelection ||
                    request.operation == PasteSelection) {
                    current = call(document, core->Snapshot, 0, "");
                    policy->observe(state, current);
                    first := state.anchor;
                    last := state.caret;
                    if (first > last) {
                        first = state.caret;
                        last = state.anchor;
                    }
                    status = core->Ok;
                    if (request.operation == ExportSelection) {
                        selected := current.text[first:last];
                        status = clipstatus(selected);
                        if (status == core->Ok) {
                            transfer = selected;
                        }
                    } else {
                        proposal := request.event;
                        status = core->Conflict;
                        if (proposal != nil && proposal.kind == Uibridge->Request &&
                            proposal.flags == current.revision && proposal.first == first && proposal.second == last) {
                            replacement := request.text;
                            if (request.operation == CutSelection) {
                                replacement = "";
                            }
                            status = core->Ok;
                            if ((request.operation == CutSelection && first != last) ||
                                (request.operation == PasteSelection && replacement != "")) {
                                status = clipstatus(replacement);
                                if (status == core->Ok) {
                                    replies := chan of Idecore->Reply;
                                    document <-= Idecore->Request(1, core->Replace, current.revision,
                                        first, last, replacement, "", replies);
                                    current = <-replies;
                                    status = current.status;
                                    policy->observe(state, current);
                                    if (status == core->Ok) {
                                        policy->select(state, current, first + len replacement, first + len replacement);
                                    }
                                }
                            }
                        }
                    }
                } else if (request.operation == Place && request.event != nil) {
                    # Refresh after an earlier rejected edit/build; its status
                    # must not make a valid unchanged-revision selection fail.
                    current = call(document, core->Snapshot, 0, "");
                    policy->observe(state, current);
                    proposal := request.event;
                    status = core->Conflict;
                    if (proposal.kind == Uibridge->Request && proposal.flags == current.revision &&
                        proposal.code >= 1 && proposal.code <= 3 && proposal.first >= 0 &&
                        proposal.first <= len current.text && proposal.second >= 0 && proposal.second <= 1) {
                        if (proposal.code == 1) {
                            pointanchor = proposal.first;
                            if (proposal.second) {
                                pointanchor = state.anchor;
                            }
                            pointing = 1;
                        }
                        if (pointing && policy->select(state, current, pointanchor, proposal.first)) {
                            status = core->Ok;
                        }
                        if (proposal.code == 3) {
                            pointing = 0;
                        }
                    } else {
                        pointing = 0;
                    }
                } else if (request.operation == Input) {
                    pointing = 0;
                    (valid, view) := policy->apply(core, document, state, request.event);
                    if (valid) {
                        current = view.document;
                        status = current.status;
                    }
                } else if (request.operation == Open) {
                    pointing = 0;
                    status = core->Conflict;
                    if (current.revision == current.saved) {
                        (readstatus, source) := files->read(sys, request.text, 65536);
                        status = core->Invalid;
                        if (readstatus == files->Ok) {
                            current = call(document, core->Close, current.revision, "");
                            if (current.status == core->Ok) {
                                current = call(document, core->Open, current.revision, source);
                                if (current.status == core->Ok) {
                                    path = request.text;
                                    policy->observe(state, current);
                                }
                            }
                            status = current.status;
                        }
                    }
                } else if (request.operation == Save) {
                    destination := request.text;
                    if (destination == "") {
                        destination = path;
                    }
                    if (files->write(sys, destination, current.text) == files->Ok) {
                        path = destination;
                        current = call(document, core->MarkSaved, current.revision, "");
                        status = current.status;
                    }
                } else if (request.operation == Build || request.operation == Diagnostic) {
                    diagnostic = "Compiler unavailable";
                    sourcekind := language(path);
                    if (builds != nil && (sourcekind != 0 || request.operation == Diagnostic)) {
                        operation := panel->Build;
                        target := Fircompiler->Swf;
                        if (sourcekind == Fircompiler->Limbo) {
                            target = Fircompiler->Dis;
                        }
                        if (request.operation == Diagnostic) {
                            operation = panel->Diagnostic;
                        }
                        answers := chan of Idebuildpanel->Reply;
                        builds <-= Idebuildpanel->Request(1, operation, current.revision, sourcekind, target, 1, answers);
                        built := <-answers;
                        diagnostic = "Build status=" + string built.status + " stage=" + string built.compiler + " " + built.diagnostic;
                        if (built.status == panel->Ok) {
                            status = core->Ok;
                            if (request.operation == Diagnostic) {
                                current = built.document;
                                policy->select(state, current, current.offset, current.selection_end);
                            }
                        } else if (built.status == panel->Conflict) {
                            status = core->Conflict;
                        }
                    } else if (builds != nil) {
                        diagnostic = "Choose .b, .as, .as1, .as2 or .as3 before Build";
                    }
                }
            }
        }
        body := text;
        first := 0;
        caret := 0;
        anchor := 0;
        if (kind == Editor) {
            (start, finish, unusedtop) := viewport->window(visible, current.text, state.caret, 1);
            first = start;
            if (state.caret > first + 200) {
                first = state.caret - 120;
            }
            last := first + 200;
            if (last > finish) {
                last = finish;
            }
            if (last < state.caret) {
                last = state.caret;
            }
            if (last > len current.text) {
                last = len current.text;
            }
            body = current.text[first:last];
            caret = state.caret - first;
            anchor = state.anchor - first;
            if (anchor < 0) {
                anchor = 0;
            }
            if (anchor > len body) {
                anchor = len body;
            }
        } else if (len body > 200) {
            if (kind == Help) {
                first = helpfirst;
            }
            if (kind == Console) {
                first = len body - 200;
            }
            last := first + 200;
            if (last > len body) {
                last = len body;
            }
            body = body[first:last];
        }
        rangefirst := state.anchor;
        rangelast := state.caret;
        if (rangefirst > rangelast) {
            rangefirst = state.caret;
            rangelast = state.anchor;
        }
        line := 1;
        if (kind == Editor) {
            for (scalar := 0; scalar < first; scalar++) {
                if (current.text[scalar] == '\n') {
                    line++;
                }
            }
        }
        colors := "";
        if (analysis != nil && kind == Editor) {
            profile := syntaxlanguage(path);
            if (profile != 0 &&
                (submittedrevision != current.revision || submittedlanguage != profile)) {
                if (syntaxticket == Sys->Maxint) {
                    raise "fail: syntax correlation exhausted";
                }
                syntaxticket++;
                colored = syntaxcall(analysis, syntax->Update,
                    Idesyntax->Request(syntaxticket, current.revision, profile, current.text, 0));
                submittedrevision = current.revision;
                submittedlanguage = profile;
            }
            colors = syntax->project(colored, current.revision, profile, first, len body);
        }
        response := Reply(request.id, status, kind, path, body, current.revision,
                          current.saved, first, caret, anchor, rangefirst, rangelast, transfer, line, diagnostic, "", colors);
        request.reply <-= response;
        if (stop) {
            return;
        }
    }
}


# Resolve implementations before exposing a new process. Missing optional native
# compilation leaves an editable window with explicit unavailable Build status.
# The returned channel owns its document until acknowledged Shutdown.
start(kind: int, path, text: string): chan of Request
{
    return startnotified(kind, path, text, nil, 0);
}


# Start one document/presentation owner and an optional private lazy analysis owner.
# Scanner/GC heaps never escape; external readiness carries only the stable identity.
startnotified(kind: int, path, text: string, notices: chan of int, identity: int): chan of Request
{
    if (kind < Editor || kind > Vector || len text > 65536) {
        return nil;
    }
    if (kind == Vector) {
        vector := load Idevector Idevector->PATH;
        sys := load Sys Sys->PATH;
        files := load Idefileops Idefileops->PATH;
        if (vector == nil || sys == nil || files == nil) {
            return nil;
        }
        doc := vector->create();
        if (text != "") {
            (status, imported) := vector->parse(text);
            if (status != vector->Ok) {
                return nil;
            }
            doc = imported;
        }
        endpoint := chan of Request;
        spawn servevector(endpoint, vector, doc, sys, files, path);
        return endpoint;
    }
    sys := load Sys Sys->PATH;
    core := load Idecore Idecore->PATH;
    policy := load Ideinput Ideinput->PATH;
    viewport := load Ideviewport Ideviewport->PATH;
    files := load Idefileops Idefileops->PATH;
    panel := load Idebuildpanel Idebuildpanel->PATH;
    workspace := load Ideworkspace Ideworkspace->PATH;
    compiler := load Idecompiler Idecompiler->PATH;
    if (sys == nil || core == nil || policy == nil || viewport == nil || files == nil || panel == nil || workspace == nil || compiler == nil) {
        return nil;
    }
    syntax: Idesyntax;
    analysis: chan of Idesyntax->Command;
    ready: chan of int;
    if (kind == Editor && notices != nil && identity > 0) {
        syntax = load Idesyntax Idesyntax->PATH;
        if (syntax != nil) {
            ready = chan[1] of int;
            analysis = syntax->startlazy(ready);
        }
    }
    requests := chan of Request;
    spawn serve(kind, path, text, requests, sys, core, policy, viewport, files, panel, workspace, compiler, syntax, analysis, ready, notices, identity);
    return requests;
}


# Own one vector document and its history independently of every text editor.
# All UI/document commands arrive through the ordinary pane channel. File calls
# may block this owner. No native graphics/VM pointer or mutable path is shared.
servevector(requests: chan of Request, vector: Idevector, doc: ref Idevector->Document,
            sys: Sys, files: Idefileops, path: string)
{
    control := 0;
    diagnostic := "";
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Idecore->Ok;
        stop := 0;
        if (request.operation == Shutdown) {
            vector->cancel(doc);
            stop = 1;
        } else if (request.operation == Draw) {
            e := request.event;
            expected := doc.revision + (len doc.points != 0);
            if (e == nil || e.kind != Uibridge->Request || e.text != "") {
                status = Idecore->Invalid;
            } else if (e.flags != expected) {
                status = Idecore->Conflict;
            } else {
                status = vectorstatus(vector->pointer(doc, e.code, e.first, e.second));
                diagnostic = "Draw status=" + string status;
            }
        } else if (request.operation == CancelPointer) {
            vector->cancel(doc);
        } else if (request.operation == Open) {
            if (doc.revision != doc.saved || len doc.points != 0 || doc.revision == 2147483647) {
                status = Idecore->Conflict;
            } else {
                (readstatus, text) := files->read(sys, request.text, 65536);
                status = vectorfilestatus(readstatus);
                if (readstatus == files->Ok) {
                    (parsed, loaded) := vector->parse(text);
                    status = vectorstatus(parsed);
                    if (parsed == vector->Ok) {
                        loaded.revision = doc.revision + 1;
                        loaded.saved = loaded.revision;
                        doc = loaded;
                        path = request.text;
                    }
                }
            }
            diagnostic = "SVG Open status=" + string status;
        } else if (request.operation == Save) {
            destination := request.text;
            if (destination == "") {
                destination = path;
            }
            if (len doc.points != 0 || destination == "") {
                status = Idecore->Conflict;
            } else {
                status = vectorfilestatus(files->write(sys, destination, vector->svg(doc)));
                if (status == Idecore->Ok) {
                    path = destination;
                    doc.saved = doc.revision;
                }
            }
            diagnostic = "SVG Save status=" + string status;
        } else if (request.operation == Input && request.event != nil) {
            e := request.event;
            if (e.kind == Uibridge->Focus && !e.flags) {
                control = 0;
                vector->cancel(doc);
            }
            if (e.kind == Uibridge->Key) {
                if (e.code == 17) {
                    control = e.flags;
                }
                if (e.flags == 1) {
                    if (e.code == 27) {
                        vector->cancel(doc);
                    } else if (control && (e.code == 90 || e.code == 89)) {
                        vector->history(doc, e.code == 89);
                    } else if (!control) {
                        vectorstyle(vector, doc, e.code);
                    }
                }
            }
            if (e.kind == Uibridge->Text && !control) {
                # SDL ordinary letters/digits arrive as asynchronous committed
                # text, possibly batched. They select tools here, never insert text.
                for (i := 0; i < len e.text; i++) {
                    key := e.text[i];
                    if (key >= 'a' && key <= 'z') {
                        key -= 'a' - 'A';
                    }
                    if (key == '+' || key == '=') {
                        key = 187;
                    } else if (key == '-') {
                        key = 189;
                    }
                    vectorstyle(vector, doc, key);
                }
            }
        } else if (request.operation == Build) {
            status = Idecore->Invalid;
            diagnostic = "Save this drawing as SVG with Ctrl+S.";
        } else if (request.operation != Snapshot && request.operation != Diagnostic) {
            status = Idecore->Invalid;
        }
        tools := array[] of {"", "Brush", "Rectangle", "Ellipse", "Line"};
        body := "B/R/E/L | 1..6 color | F fill\n" + tools[doc.tool]
            + sys->sprint(" #%06x w=%d f=%d | +/- Z/Y", doc.color, doc.width, doc.fill);
        revision := doc.revision;
        if (len doc.points != 0) {
            revision++;
        }
        request.reply <-= Reply(request.id, status, Vector, path, body, revision,
            doc.saved, 0, -1, -1, 0, 0, "", 0, diagnostic, vector->wire(doc), "");
        if (stop) {
            return;
        }
    }
}


# Apply one copied authoring key to a borrowed pane-owned document. Tool/style
# changes require an idle gesture and do not alter source, history or saved state.
# Used by explicit Key and asynchronous/batched SDL Text input; no UI/I/O occurs.
vectorstyle(vector: Idevector, doc: ref Idevector->Document, key: int)
{
    if (len doc.points != 0) {
        return;
    }
    if (key == 66) {
        doc.tool = vector->Brush;
    } else if (key == 82) {
        doc.tool = vector->Rectangle;
    } else if (key == 69) {
        doc.tool = vector->Ellipse;
    } else if (key == 76) {
        doc.tool = vector->Line;
    } else if (key == 70) {
        doc.fill = !doc.fill;
    } else if (key >= 49 && key <= 54) {
        palette := array[] of {16r245ac4, 16re04444, 16r229955, 16rf0b323, 16r222222, 16r9933bb};
        doc.color = palette[key - 49];
    } else if ((key == 187 || key == 107) && doc.width < 32) {
        doc.width++;
    } else if ((key == 189 || key == 109) && doc.width > 1) {
        doc.width--;
    }
}


# Translate the pure model's local status namespace into the shared pane contract.
# No model constants are leaked as unrelated Idecore status values.
vectorstatus(status: int): int
{
    if (status == Idevector->Ok) {
        return Idecore->Ok;
    }
    if (status == Idevector->Limit) {
        return Idecore->TooLarge;
    }
    if (status == Idevector->Conflict) {
        return Idecore->Conflict;
    }
    return Idecore->Invalid;
}


# Map checked file outcomes to the pane's existing public status namespace.
# Native I/O has no separate Idecore code: its failure maps to Invalid.
vectorfilestatus(status: int): int
{
    if (status == Idefileops->Ok) {
        return Idecore->Ok;
    }
    if (status == Idefileops->TooLarge) {
        return Idecore->TooLarge;
    }
    return Idecore->Invalid;
}
