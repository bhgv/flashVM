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
    colored := Idesyntax->Result(0, 0, 0, Idesyntax->Pending, nil);
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
                          current.saved, first, caret, anchor, rangefirst, rangelast, transfer, line, diagnostic, colors);
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
    if (kind < Editor || kind > Help || len text > 65536) {
        return nil;
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
