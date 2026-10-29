implement DesktopIDE;
include "sys.m";
include "uibridge.m";
include "idecore.m";
include "idefileops.m";
include "ideinput.m";
include "idefield.m";
include "idehost.m";
include "idedeskpane.m";
include "idedesktop.m";
DesktopIDE: module
{
    init: fn(); # Own the MovieClip desktop and independent pane processes.
};


# Complete one private window exchange; every admitted reply is consumed.
panecall(owner: chan of Idedeskpane->Request, operation: int, text: string,
         event: ref Uibridge->Message): Idedeskpane->Reply
{
    replies := chan of Idedeskpane->Reply;
    owner <-= Idedeskpane->Request(1, operation, text, event, replies);
    return <-replies;
}


# Complete one private layout-owner exchange without exposing mutable state.
deskcall(owner: chan of Idedesktop->Request, request: Idedesktop->Request): Idedesktop->Reply
{
    request.reply = chan of Idedesktop->Reply;
    owner <-= request;
    return <-request.reply;
}


# Encode one copied view value; output blocks until the actual host consumes it.
send(codec: Uibridge, output: chan of array of byte, code, flags, first, last: int, text: string)
{
    (valid, wire) := codec->encode(ref Uibridge->Message(codec->View, 0, code, flags, first, last, text));
    if (!valid) {
        raise "fail: desktop scene encoding";
    }
    output <-= wire;
}


# Keep source identities readable without changing the actual destination path.
caption(reply: Idedeskpane->Reply, index: int): string
{
    title := "Editor " + string index;
    if (reply.kind == Idedeskpane->Console) {
        title = "Build console";
    } else if (reply.kind == Idedeskpane->Help) {
        title = "Help / Справка";
    } else if (reply.path != "") {
        title = reply.path;
    }
    if (len title > 28) {
        title = title[len title - 28:];
    }
    if (reply.revision != reply.saved) {
        title = "* " + title;
    }
    return title;
}


# Own the application channel registry, not window text. Each editor/log/help has
# its own spawned Limbo controller; all MovieClip capabilities stay on FlashVM.
# Native compilation/I/O remain blocking primitives. No parallel native VM claim.
init()
{
    sys := load Sys Sys->PATH;
    codec := load Uibridge Uibridge->PATH;
    host := load Idehost Idehost->PATH;
    panes := load Idedeskpane Idedeskpane->PATH;
    desk := load Idedesktop Idedesktop->PATH;
    fields := load Idefield Idefield->PATH;
    policy := load Ideinput Ideinput->PATH;
    files := load Idefileops Idefileops->PATH;
    if (sys == nil || codec == nil || host == nil || panes == nil || desk == nil || fields == nil || policy == nil || files == nil) {
        raise "fail: desktop modules";
    }
    windows := array[desk->MaxWindows] of chan of Idedeskpane->Request;
    path := host->path();
    source := "";
    if (path != "" && !host->fresh()) {
        (status, loaded) := files->read(sys, path, 65536);
        if (status != files->Ok) {
            raise "fail: desktop initial source";
        }
        source = loaded;
    }
    windows[0] = panes->start(panes->Editor, path, source);
    windows[1] = panes->start(panes->Console, "", "Build output appears here. F7 compiles the focused editor through its own channel.");
    windows[2] = panes->start(panes->Help, "", "F1: Help. PageUp/PageDown: help pages. F7: Build. F8: source diagnostic. Ctrl+S: Save. Ctrl+O: Open. Ctrl+N: New editor. Ctrl+W: Close editor.\nDrag titles to move windows. Click a window to focus. Drag EMPTY table space to pan. Left thumbnails jump. Tools expand on the right.\n\nFiles: Open protects unsaved edits. Save records the active editor only. New keeps existing editors open.\n\nBuild: choose a .b, .as, .as1, .as2 or .as3 path first. The build console records the result. F8 selects the latest source diagnostic.\n\nClose: dirty editors offer Discard this editor or Cancel/Escape. Help and console stay open.\n\nQuit: unsaved editors require an explicit Discard and close action. Debugger sessions are not attached in this shell yet.");
    windows[3] = panes->start(panes->Editor, "", "");
    for (index := 0; index < 4; index++) {
        if (windows[index] == nil) {
            raise "fail: desktop pane startup";
        }
    }
    layout := desk->start();
    request := Idedesktop->Request(1, desk->View, 0, 0, 0, 0, 0, "", "", -1, nil);
    view := deskcall(layout, request);
    input := chan of array of byte;
    output := chan of array of byte;
    if (host->attach(input, output) != 0) {
        raise "fail: desktop host registration";
    }
    width := 960;
    height := 640;
    active := 1;
    control := 0;
    shift := 0;
    modal := 0;
    closing := 0; # Stable editor slot retained while its dirty-close decision is modal.
    field := fields->create();
    finished := 0;
    for (;;) {
        dirty := 0;
        for (index = 0; index < len windows; index++) {
            if (windows[index] == nil) {
                continue;
            }
            snapshot := panecall(windows[index], panes->Snapshot, "", nil);
            dirty += snapshot.revision != snapshot.saved;
            request.operation = desk->Update;
            request.window = index + 1;
            request.title = caption(snapshot, index + 1);
            request.body = snapshot.body;
            request.caret = -1;
            if (snapshot.kind == panes->Editor) {
                request.caret = snapshot.caret;
            }
            view = deskcall(layout, request);
        }
        if (finished) {
            break;
        }
        if (view.focus != active) {
            request.operation = desk->Select;
            request.window = active;
            view = deskcall(layout, request);
        }
        request.operation = desk->View;
        view = deskcall(layout, request);
        count := len view.items;
        if (modal != 0) {
            count++;
        }
        send(codec, output, desk->BeginView, 0, width, height,
             codec->hexword(view.ticket) + codec->hexword(count) + codec->hexword(view.scroll));
        for (index = 0; index < len view.items; index++) {
            item := view.items[index];
            metadata := codec->hexword(item.parent) + codec->hexword(item.x) + codec->hexword(item.y)
                + codec->hexword(item.width) + codec->hexword(item.height) + codec->hexword(item.style)
                + codec->hexword(item.caret) + codec->hexword(item.mirror);
            send(codec, output, desk->ItemView, view.ticket, item.id, 0, metadata + item.text);
        }
        if (modal != 0) {
            x := width - 416;
            if (x < 92) {
                x = 92;
            }
            wide := width - 128 - x;
            if (wide < 24) {
                wide = 24;
            }
            first := field.caret - 120;
            if (first < 0) {
                first = 0;
            }
            last := first + 200;
            if (last > len field.text) {
                last = len field.text;
            }
            send(codec, output, desk->ItemView, view.ticket, 57, 0, codec->hexword(4) + codec->hexword(x)
                + codec->hexword(240) + codec->hexword(wide) + codec->hexword(64)
                + codec->hexword(3) + codec->hexword(field.caret - first) + codec->hexword(0) + field.text[first:last]);
        }
        send(codec, output, desk->EndView, view.ticket, 0, 0, "");
        (valid, event) := codec->decode(<-input);
        if (!valid) {
            raise "fail: desktop event envelope";
        }
        if ((event.kind == codec->Key && (event.flags < 0 || event.flags > 1 || event.code < 1 || event.code > 255)) ||
            (event.kind == codec->Focus && (event.flags < 0 || event.flags > 1))) {
            raise "fail: desktop input flags";
        }
        action := 0;
        approved := 0; # Only an admitted click on the explicit discard control.
        if (event.kind == codec->Request && event.code == host->Quit) {
            if (!dirty || event.flags == 1) {
                finished = 1;
                continue;
            }
            closing = 0;
            request.operation = desk->Tool;
            request.window = desk->Quit;
            view = deskcall(layout, request);
            panecall(windows[1], panes->SetText, "Unsaved editors remain open. Save them, or explicitly execute Quit to discard.", nil);
            continue;
        }
        if (event.kind == codec->Request && event.code == desk->ResizeEvent && event.flags == view.ticket) {
            request.operation = desk->Resize;
            request.x = event.first;
            request.y = event.second;
            view = deskcall(layout, request);
            if (view.status == desk->Ok) {
                width = event.first;
                height = event.second;
            }
            continue;
        }
        if (event.kind == codec->Key && event.code == 17) {
            control = event.flags;
        }
        if (event.kind == codec->Key && event.code == 16) {
            shift = event.flags;
        }
        if (event.kind == codec->Focus && !event.flags) {
            control = 0;
            shift = 0;
            request.operation = desk->Cancel;
            view = deskcall(layout, request);
        }
        if (closing != 0) {
            # Only explicit close/cancel controls may resolve this decision.
            # No keyboard edit, focus switch or new file may alter its target.
            if (event.kind == codec->Key && event.flags == 1 && event.code == 27) {
                closing = 0;
                request.operation = desk->Tool;
                request.window = 0;
                view = deskcall(layout, request);
                continue;
            }
            left := width - 424;
            if (left < 84) {
                left = 84;
            }
            if (event.kind != codec->Request || event.code != desk->PointerEvent ||
                event.first < left + 8 || event.first >= width - 120 ||
                !((event.second >= 320 && event.second < 356) ||
                  (event.second >= 364 && event.second < 400))) {
                continue;
            }
        }
        if (modal != 0) {
            if (event.kind == codec->Key && event.flags == 1 && event.code == 27) {
                modal = 0;
                request.operation = desk->Tool;
                request.window = 0;
                view = deskcall(layout, request);
            } else if (event.kind == codec->Key && event.flags == 1 && event.code == 13) {
                operation := panes->Open;
                if (modal == desk->SaveFile) {
                    operation = panes->Save;
                }
                result := panecall(windows[active - 1], operation, field.text, nil);
                panecall(windows[1], panes->SetText, "File status=" + string result.status + " " + result.path, nil);
                if (result.status == Idecore->Ok) {
                    modal = 0;
                    request.operation = desk->Tool;
                    request.window = 0;
                    view = deskcall(layout, request);
                }
            } else {
                fields->apply(policy, field, event, control, shift);
            }
            continue;
        }
        if (event.kind == codec->Request && event.code == desk->PointerEvent && len array of byte event.text == 16) {
            (phaseok, phase) := codec->parseword(array of byte event.text, 0);
            (codeok, code) := codec->parseword(array of byte event.text, 8);
            if (phaseok && codeok && (code == 1 || code == 4 || code == 5)) {
                request.ticket = event.flags;
                request.operation = desk->Pointer;
                request.x = event.first;
                request.y = event.second;
                request.phase = phase;
                if (code == 4 || code == 5) {
                    request.operation = desk->Scroll;
                    request.x = 120;
                    if (code == 4) {
                        request.x = -120;
                    }
                }
                previousdrawer := view.drawer;
                view = deskcall(layout, request);
                action = view.action;
                left := width - 424;
                if (left < 84) {
                    left = 84;
                }
                approved = action == desk->CloseEditor && previousdrawer == desk->CloseEditor &&
                    phase == 1 && code == 1 && event.first >= left + 8 && event.first < width - 120 &&
                    event.second >= 320 && event.second < 356;
                if (action == desk->Focus) {
                    panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                    active = view.focus;
                    control = 0;
                    shift = 0;
                    continue;
                }
            }
        } else if (event.kind == codec->Key && event.flags == 1) {
            if (event.code == 112) {
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                active = 3;
                control = 0;
                shift = 0;
                request.operation = desk->Tool;
                request.window = desk->Help;
                view = deskcall(layout, request);
                continue;
            }
            if (event.code == 27) {
                request.operation = desk->Tool;
                request.window = 0;
                view = deskcall(layout, request);
                continue;
            }
            if (event.code == 118) {
                action = desk->Compile;
            } else if (control && event.code == 83) {
                action = desk->SaveFile;
            } else if (control && event.code == 79) {
                action = desk->OpenFile;
            } else if (control && event.code == 78) {
                action = desk->NewEditor;
            } else if (control && event.code == 87) {
                action = desk->CloseEditor;
            } else if (event.code == 119) {
                result := panecall(windows[active - 1], panes->Diagnostic, "", nil);
                panecall(windows[1], panes->SetText, result.diagnostic, nil);
                continue;
            }
        }
        if (action == desk->CancelClose) {
            closing = 0;
        } else if (action == desk->CloseEditor) {
            # A title button may target an inactive window. Change focus only
            # after layout admission and preserve every other document owner.
            if (view.focus != active) {
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                active = view.focus;
            }
            current := panecall(windows[active - 1], panes->Snapshot, "", nil);
            if (current.kind != panes->Editor) {
                panecall(windows[1], panes->SetText, "Help and build console stay open. Select an editor to close.", nil);
            } else if (current.revision != current.saved && closing == 0 && !approved) {
                closing = active;
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                request.operation = desk->Tool;
                request.window = desk->CloseEditor;
                view = deskcall(layout, request);
            } else {
                # Consume the terminal acknowledgement before reusing the slot.
                panecall(windows[active - 1], panes->Shutdown, "", nil);
                windows[active - 1] = nil;
                request.operation = desk->Remove;
                request.window = active;
                view = deskcall(layout, request);
                active = view.focus;
                closing = 0;
            }
            control = 0;
            shift = 0;
        } else if (action == desk->OpenFile || action == desk->SaveFile) {
            current := panecall(windows[active - 1], panes->Snapshot, "", nil);
            if (current.kind == panes->Editor) {
                if (action == desk->SaveFile && current.path != "" && !shift) {
                    result := panecall(windows[active - 1], panes->Save, "", nil);
                    panecall(windows[1], panes->SetText, "Save status=" + string result.status, nil);
                } else {
                    modal = action;
                    panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                    field = fields->create();
                    fields->set(field, current.path, 1);
                    request.operation = desk->Tool;
                    request.window = action;
                    view = deskcall(layout, request);
                }
            }
        } else if (action == desk->NewEditor) {
            created := 0;
            for (index = 0; index < len windows; index++) {
                if (windows[index] == nil) {
                    owner := panes->start(panes->Editor, "", "");
                    if (owner == nil) {
                        panecall(windows[1], panes->SetText, "Cannot start an editor owner.", nil);
                        break;
                    }
                    windows[index] = owner;
                    created = 1;
                    panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                    active = index + 1;
                    control = 0;
                    shift = 0;
                    break;
                }
            }
            if (!created && index == len windows) {
                panecall(windows[1], panes->SetText, "Desktop window limit reached (8).", nil);
            }
        } else if (action == desk->Compile) {
            result := panecall(windows[active - 1], panes->Build, "", nil);
            panecall(windows[1], panes->SetText, result.diagnostic, nil);
        } else if (action == desk->Help) {
            panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
            active = 3;
            control = 0;
            shift = 0;
            request.operation = desk->Tool;
            request.window = desk->Help;
            view = deskcall(layout, request);
        } else if (action == desk->Quit) {
            finished = 1;
        } else if (event.kind == codec->Key || event.kind == codec->Text || event.kind == codec->Focus) {
            panecall(windows[active - 1], panes->Input, "", event);
        }
    }
    for (index = 0; index < len windows; index++) {
        if (windows[index] != nil) {
            stopped := panecall(windows[index], panes->Shutdown, "", nil);
            if (stopped.status != Idecore->Ok) {
                raise "fail: desktop pane shutdown";
            }
        }
    }
    request.operation = desk->Shutdown;
    stopped := deskcall(layout, request);
    if (stopped.status != desk->Ok) {
        raise "fail: desktop layout shutdown";
    }
    host->detach();
    sys->print("PASS MovieClip desktop: independent panes and acknowledged shutdown\n");
}
