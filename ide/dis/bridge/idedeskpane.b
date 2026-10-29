implement Idedeskpane;
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


# Own one editor/log/help process. All dependencies and mutable document/input
# state remain on this stack. Native compilation/file I/O block the VM owner;
# separate Limbo processes do not imply parallel native parser or UI execution.
serve(kind: int, path, text: string, requests: chan of Request, sys: Sys,
      core: Idecore, policy: Ideinput, viewport: Ideviewport, files: Idefileops, panel: Idebuildpanel,
      workspace: Ideworkspace, compiler: Idecompiler)
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
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := core->Invalid;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Shutdown) {
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
                if (request.operation == Input) {
                    (valid, view) := policy->apply(core, document, state, request.event);
                    if (valid) {
                        current = view.document;
                        status = current.status;
                    }
                } else if (request.operation == Open) {
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
        response := Reply(request.id, status, kind, path, body, current.revision,
                          current.saved, first, caret, diagnostic);
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
    requests := chan of Request;
    spawn serve(kind, path, text, requests, sys, core, policy, viewport, files, panel, workspace, compiler);
    return requests;
}
