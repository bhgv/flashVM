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
include "idedeskclip.m";
include "idedebug.m";
include "idemdi.m";
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


# Exchange one membership operation. The private registry owns only copied pane
# identities/tool families; file/document processes remain independently owned.
mdicall(owner: chan of Idemdi->Request, revision, operation, document, family: int): Idemdi->Reply
{
    replies := chan of Idemdi->Reply;
    owner <-= Idemdi->Request(1, revision, operation, document, 0, family, replies);
    return <-replies;
}


# Exchange a complete membership command. owner is borrowed; revision is exact,
# operation is an Idemdi command, document/container are stable bounded IDs,
# and family is Source/Graphics for creation. Consume the copied reply before
# changing presentation. No document/file/native ownership is transferred.
mdigroupcall(owner: chan of Idemdi->Request, revision, operation, document, container, family: int): Idemdi->Reply
{
    replies := chan of Idemdi->Reply;
    owner <-= Idemdi->Request(1, revision, operation, document, container, family, replies);
    return <-replies;
}


# Complete one private layout-owner exchange without exposing mutable state.
deskcall(owner: chan of Idedesktop->Request, request: Idedesktop->Request): Idedesktop->Reply
{
    request.reply = chan of Idedesktop->Reply;
    owner <-= request;
    return <-request.reply;
}


# Exchange one private session clipboard value. owner is borrowed; text is an
# immutable copy and expected guards writes. Consume the complete acknowledgement
# before Cut may edit a document. No native/OS clipboard capability is acquired.
clipcall(owner: chan of Idedeskclip->Request, operation, expected: int, text: string): Idedeskclip->Reply
{
    replies := chan of Idedeskclip->Reply;
    owner <-= Idedeskclip->Request(1, operation, expected, text, replies);
    return <-replies;
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


# Interpret one decoded uint32 hit word as signed int32 on classic and NC64.
# parseword intentionally returns raw bits; toolbar cells use negative sentinels.
# Return only a copied scalar, without I/O, source mutation or native access.
signedoffset(value: int): int
{
    number := big value;
    if (number >= big 16r80000000) {
        number -= big 16r100000000;
    }
    return int number;
}


# Keep source identities readable without changing the actual destination path.
caption(reply: Idedeskpane->Reply, index: int): string
{
    title := "Editor " + string index;
    if (reply.kind == Idedeskpane->Debugger) {
        return reply.path;
    }
    if (reply.kind == Idedeskpane->Console) {
        title = "Build console";
    } else if (reply.kind == Idedeskpane->Help) {
        title = "Help / Справка";
    } else if (reply.kind == Idedeskpane->Vector && reply.path == "") {
        title = "Vector " + string index + ".svg";
    } else if (reply.path != "") {
        title = reply.path;
    }
    # Captions are one visual line. Preserve the real path while replacing only
    # presentation controls, including valid filename line/paragraph separators.
    for (scalar := 0; scalar < len title; scalar++) {
        value := title[scalar];
        if (value < 32 || value == 127 || value == 16r85 || value == 16r2028 || value == 16r2029) {
            title[scalar] = ' ';
        }
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
    clips := load Idedeskclip Idedeskclip->PATH;
    files := load Idefileops Idefileops->PATH;
    if (sys == nil || codec == nil || host == nil || panes == nil || desk == nil || fields == nil || policy == nil || files == nil || clips == nil) {
        raise "fail: desktop modules";
    }
    notices := chan[8] of int; # Stable pane identities only; snapshots remain owner-authoritative.
    windows := array[desk->MaxWindows] of chan of Idedeskpane->Request;
    shown := array[desk->MaxWindows] of Idedeskpane->Reply;
    drawing := 0; # Stable vector owner receiving the current admitted single-contact gesture.
    pointing := 0; # Stable source slot for the admitted body selection gesture.
    path := host->path();
    source := "";
    if (path != "" && !host->fresh()) {
        (status, loaded) := files->read(sys, path, 65536);
        if (status != files->Ok) {
            raise "fail: desktop initial source";
        }
        source = loaded;
    }
    initialkind := panes->Editor;
    if (len path >= 4 && path[len path - 4:] == ".svg") {
        initialkind = panes->Vector;
    }
    windows[0] = panes->startnotified(initialkind, path, source, notices, 1);
    windows[1] = panes->start(panes->Console, "", "Build output appears here. F7 compiles the focused editor through its own channel.");
    windows[2] = panes->start(panes->Help, "", "F1 Help; PageUp/Down pages. Drag a header to move. Double-tap/click a header or Max/N to maximize/restore. Ctrl+wheel/pinch: header sizes a window; table scales all. All fits every window. Dock when only20px remain beyond the left rail; transitions ease over240ms. Tap/swipe a miniature right to reveal the entire pane by smoothly panning the table; drag it vertically to reorder. Two fingers move a window; three pan the table. The right Help button hides/restores this pane.\n\nKeyboard: F7: Build. F8: source diagnostic. Ctrl+S: Save. Ctrl+O: Open. Ctrl+N: New editor. Ctrl+W: Close editor. Ctrl+C/X/V: session clipboard. Click text to place the caret. Shift-click or drag selects.\nDrag titles to move windows. Click a window to focus. Drag EMPTY table space to pan. Left thumbnails jump. Tools expand on the right.\n\nDraw / Ctrl+Shift+N creates a vector window. Upper row: tools, Undo/Redo, width. Lower row: colours, fill, width indicator. B/R/E/L: brush/rectangle/ellipse/line.1..6: colour. F: fill. +/-: width. Ctrl+Z/Y: Undo/Redo. Ctrl+S/O: SVG Save/Open.\n\nMDI / Ctrl+G groups this source or drawing. Shift+MDI / Ctrl+Shift+G creates a separate matching container. SDI / Ctrl+D detaches with saved geometry. Drag an exposed container title; children are tiled in this first version.\n\nSyntax: lazy Coco/R Limbo/AS colours run independently of Flash frames. Unknown suffixes stay plain.\n\nFiles: Open protects unsaved edits. Save records the active editor only. New keeps existing editors open.\n\nBuild: choose a .b, .as, .as1, .as2 or .as3 path first. The build console records the result. F8 selects the latest source diagnostic.\n\nClose: dirty editors offer Discard this editor or Cancel/Escape. Help and console owners remain alive.\n\nQuit: unsaved editors require an explicit Discard and close action. Debug opens source/stack/data panels on this same desktop. No target is attached yet.");
    windows[3] = panes->startnotified(panes->Editor, "", "", notices, 4);
    for (index := 0; index < 4; index++) {
        if (windows[index] == nil) {
            raise "fail: desktop pane startup";
        }
    }
    debugger := load Idedebug Idedebug->PATH;
    if (debugger == nil) {
        raise "fail: debugger session module";
    }
    session := debugger->start(1);
    mdi := load Idemdi Idemdi->PATH;
    if (mdi == nil) {
        raise "fail: MDI registry module";
    }
    membership := mdi->start();
    mdirevision := 1;
    for (index = 0; index < 4; index++) {
        if (index == 1 || index == 2) {
            continue;
        }
        family := mdi->Source;
        if (index == 0 && initialkind == panes->Vector) {
            family = mdi->Graphics;
        }
        registered := mdicall(membership, mdirevision, mdi->Register, index + 1, family);
        if (registered.status != mdi->Ok) {
            raise "fail: initial MDI registration";
        }
        mdirevision = registered.revision;
    }
    debugpanes := array[] of {0, 0, 0}; # Stable readonly source/stack/data panel identities.
    clipboard := clips->start();
    layout := desk->start();
    request := Idedesktop->Request(1, desk->View, 0, 0, 0, 0, 0, "", "", -1, -1, 0, nil);
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
    refresh := 1; # Geometry-only motion reuses immutable pane snapshots.
    dirty := 0; # Last source-authoritative dirty count, refreshed after edits.
    for (;;) {
        if (refresh) {
            dirty = 0;
        }
        for (index = 0; index < len windows; index++) {
            if (!refresh || windows[index] == nil) {
                continue;
            }
            snapshot := panecall(windows[index], panes->Snapshot, "", nil);
            shown[index] = snapshot;
            dirty += snapshot.revision != snapshot.saved;
            request.operation = desk->Update;
            request.window = index + 1;
            request.title = caption(snapshot, index + 1);
            request.body = snapshot.body;
            if (snapshot.kind == panes->Vector) {
                request.body = ""; # The copied toolbar replaces the text-only legend.
            }
            request.caret = -1;
            request.anchor = -1;
            request.line = 0;
            if (snapshot.kind == panes->Editor) {
                request.line = snapshot.line;
                request.caret = snapshot.caret;
                request.anchor = snapshot.anchor;
            }
            view = deskcall(layout, request);
        }
        refresh = 1;
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
             codec->hexword(view.ticket) + codec->hexword(count) + codec->hexword(view.scroll) + codec->hexword(view.zoom) + codec->hexword(view.pan) + codec->hexword(view.reveal));
        for (index = 0; index < len view.items; index++) {
            item := view.items[index];
            metadata := codec->hexword(item.parent) + codec->hexword(item.x) + codec->hexword(item.y)
                + codec->hexword(item.width) + codec->hexword(item.height) + codec->hexword(item.style)
                + codec->hexword(item.caret) + codec->hexword(item.mirror) + codec->hexword(item.anchor);
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
                + codec->hexword(3) + codec->hexword(field.caret - first) + codec->hexword(0)
                + codec->hexword(-1) + field.text[first:last]);
        }
        for (index = 0; index < len view.items; index++) {
            id := view.items[index].id;
            if (id >= 1 && id <= len windows && windows[id-1] != nil &&
                shown[id-1].colors != "") {
                send(codec, output, desk->ColorsView, view.ticket, id,
                    shown[id-1].revision, shown[id-1].colors);
            }
        }
        for (index = 0; index < len windows; index++) {
            if (windows[index] == nil || shown[index].drawing == "") {
                continue;
            }
            stream := shown[index].drawing;
            send(codec, output, desk->VectorBegin, view.ticket, index + 1, len stream,
                 codec->hexword(shown[index].revision) + codec->hexword(desk->VectorVersion));
            for (offset := 0; offset < len stream; offset += 1024) {
                end := offset + 1024;
                if (end > len stream) {
                    end = len stream;
                }
                send(codec, output, desk->VectorData, view.ticket, index + 1, offset, stream[offset:end]);
            }
        }
        send(codec, output, desk->EndView, view.ticket, 0, 0, "");
        raw: array of byte;
        for (;;) {
            receivedinput := 0;
            alt {
            raw = <-input => receivedinput = 1;
            id := <-notices =>
                if (id < 1 || id > len windows || windows[id-1] == nil) {
                    continue;
                }
                fresh := panecall(windows[id-1], panes->Snapshot, "", nil);
                if (fresh.revision != shown[id-1].revision || fresh.first != shown[id-1].first ||
                    fresh.body != shown[id-1].body || fresh.colors == "") {
                    continue;
                }
                # Paint-only publication keeps the completed geometry/input ticket.
                # It cannot acknowledge an outstanding keyboard edit or move a pane.
                shown[id-1].colors = fresh.colors;
                for (item := 0; item < len view.items; item++) {
                    if (view.items[item].id == id) {
                        send(codec, output, desk->ColorsView, view.ticket, id,
                            fresh.revision, fresh.colors);
                        break;
                    }
                }
            }
            if (receivedinput) {
                break;
            }
        }
        (valid, event) := codec->decode(raw);
        if (!valid) {
            raise "fail: desktop event envelope";
        }
        if ((event.kind == codec->Key && (event.flags < 0 || event.flags > 1 || event.code < 1 || event.code > 255)) ||
            (event.kind == codec->Focus && (event.flags < 0 || event.flags > 1))) {
            raise "fail: desktop input flags";
        }
        if (event.kind == codec->Request && event.code == desk->AnimationClock) {
            # Only presentation time advances; preserve document/gesture owners.
            continue;
        }
        if (pointing != 0 && !(event.kind == codec->Request && event.code == desk->PointerEvent &&
                              len array of byte event.text == 32)) {
            panecall(windows[pointing - 1], panes->CancelPointer, "", nil);
            pointing = 0;
        }
        if (drawing != 0 && !(event.kind == codec->Request && event.code == desk->PointerEvent)) {
            panecall(windows[drawing - 1], panes->CancelPointer, "", nil);
            drawing = 0;
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
            if (!control) {
                request.operation = desk->Cancel;
                view = deskcall(layout, request);
            }
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
        if (event.kind == codec->Request && event.code == desk->PointerEvent && len array of byte event.text == 32) {
            (phaseok, phase) := codec->parseword(array of byte event.text, 0);
            (codeok, code) := codec->parseword(array of byte event.text, 8);
            (hitok, hit) := codec->parseword(array of byte event.text, 16);
            (offsetok, offset) := codec->parseword(array of byte event.text, 24);
            offset = signedoffset(offset);
            if (phaseok && codeok && hitok && offsetok && (code == 1 || code == 4 || code == 5 || code == 6 || code == 7 || code == 8 || code == 9 || code == 10)) {
                if (drawing != 0 && code != 1) {
                    panecall(windows[drawing-1], panes->CancelPointer, "", nil);
                    drawing = 0;
                }
                if (code == 1 && hit >= 1 && hit <= len windows && windows[hit-1] != nil &&
                    shown[hit-1].kind == panes->Vector && pointing != 0) {
                    panecall(windows[pointing-1], panes->CancelPointer, "", nil);
                    pointing = 0;
                }
                if (code == 1 && (drawing != 0 || (hit >= 1 && hit <= len windows &&
                    windows[hit-1] != nil && shown[hit-1].kind == panes->Vector))) {
                    if (event.flags != view.ticket || hit < 1 || hit > len windows ||
                        windows[hit-1] == nil || shown[hit-1].kind != panes->Vector ||
                        offset < -16 || offset == -1 || offset > 601600 || (drawing != 0 && drawing != hit)) {
                        if (drawing != 0) {
                            panecall(windows[drawing-1], panes->CancelPointer, "", nil);
                            drawing = 0;
                        }
                        continue;
                    }
                    request.operation = desk->TextHit;
                    request.ticket = event.flags;
                    request.window = hit;
                    request.x = event.first;
                    request.y = event.second;
                    admitted := deskcall(layout, request);
                    if (admitted.status != desk->Ok || (phase != 1 && drawing != hit)) {
                        continue;
                    }
                    if (offset < -1) {
                        if (drawing != 0) {
                            panecall(windows[drawing-1], panes->CancelPointer, "", nil);
                            drawing = 0;
                            continue;
                        }
                        if (phase == 1) {
                            if (active != hit) {
                                panecall(windows[active-1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                            }
                            active = hit;
                            request.operation = desk->Activate;
                            view = deskcall(layout, request);
                            panecall(windows[hit-1], panes->VectorControl, "",
                                ref Uibridge->Message(codec->Request, 0, -offset - 1, shown[hit-1].revision, 0, 0, ""));
                        }
                        continue;
                    }
                    if (phase == 1) {
                        if (active != hit) {
                            panecall(windows[active-1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                        }
                        active = hit;
                        drawing = hit;
                        request.operation = desk->Activate;
                        view = deskcall(layout, request);
                    }
                    result := panecall(windows[hit-1], panes->Draw, "",
                        ref Uibridge->Message(codec->Request, 0, phase, shown[hit-1].revision,
                            offset % 1001, offset / 1001, ""));
                    if (phase == 3 || result.status != Idecore->Ok) {
                        panecall(windows[hit-1], panes->CancelPointer, "", nil);
                        drawing = 0;
                    }
                    continue;
                }
                request.ticket = event.flags;
                request.operation = desk->Pointer;
                request.x = event.first;
                request.y = event.second;
                request.phase = phase;
                if (code == 6 || code == 7) {
                    request.phase += 4 * (code - 4);
                }
                if (code == 8 || code == 9 || code == 10) {
                    request.phase = code - 4;
                }
                if (code == 4 || code == 5) {
                    request.operation = desk->Scroll;
                    request.x = 120;
                    if (code == 4) {
                        request.x = -120;
                    }
                }
                if (phase == 2 && pointing == 0 && (code == 1 || code == 6 || code == 7)) {
                    refresh = 0;
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
                # Native glyph geometry proposes an offset. Layout independently
                # admits its pane, and that pane checks the shown source revision.
                placed := 0;
                if (view.status == desk->Ok && code == 1 && hit >= 1 && hit <= len windows &&
                    windows[hit - 1] != nil && shown[hit - 1].kind == panes->Editor && closing == 0) {
                    prefix := len caption(shown[hit - 1], hit) + 1;
                    if (offset >= prefix && offset <= prefix + len shown[hit - 1].body &&
                        (phase == 1 || pointing == hit)) {
                        request.operation = desk->TextHit;
                        request.ticket = view.ticket;
                        request.window = hit;
                        admitted := deskcall(layout, request);
                        if (admitted.status == desk->Ok) {
                            extend := shift;
                            if (phase == 1) {
                                if (active != hit) {
                                    panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                                    active = hit;
                                    control = 0;
                                    shift = 0;
                                }
                                request.operation = desk->Activate;
                                request.window = hit;
                                view = deskcall(layout, request);
                                pointing = hit;
                            }
                            result := panecall(windows[hit - 1], panes->Place, "",
                                ref Uibridge->Message(codec->Request, 0, phase, shown[hit - 1].revision,
                                    shown[hit - 1].first + offset - prefix, extend, ""));
                            placed = result.status == Idecore->Ok;
                        }
                    }
                }
                if (pointing != 0 && (!placed || phase == 3 || code != 1)) {
                    panecall(windows[pointing - 1], panes->CancelPointer, "", nil);
                    pointing = 0;
                }
                if (placed) {
                    continue;
                }
                if (action == desk->Focus) {
                    panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                    active = view.focus;
                    control = 0;
                    shift = 0;
                    continue;
                }
            } else if (pointing != 0) {
                panecall(windows[pointing - 1], panes->CancelPointer, "", nil);
                pointing = 0;
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
                if (shift) {
                    action = desk->VectorEditor;
                }
            } else if (control && event.code == 71) {
                action = desk->GroupEditor;
            } else if (control && event.code == 68) {
                action = desk->DetachEditor;
            } else if (control && event.code == 67) {
                action = desk->CopySelection;
            } else if (control && event.code == 88) {
                action = desk->CutSelection;
            } else if (control && event.code == 86) {
                action = desk->PasteSelection;
            } else if (control && event.code == 87) {
                action = desk->CloseEditor;
            } else if (event.code == 119) {
                result := panecall(windows[active - 1], panes->Diagnostic, "", nil);
                panecall(windows[1], panes->SetText, result.diagnostic, nil);
                continue;
            }
        }
        if (action == desk->CopySelection || action == desk->CutSelection || action == desk->PasteSelection) {
            current := panecall(windows[active - 1], panes->Snapshot, "", nil);
            if (current.kind != panes->Editor) {
                panecall(windows[1], panes->SetText, "Session clipboard: select an editor first.", nil);
            } else {
                stored := clipcall(clipboard, clips->Get, 0, "");
                status := current.status;
                note := "";
                if (action == desk->PasteSelection) {
                    copied := panecall(windows[active - 1], panes->PasteSelection, stored.text,
                        ref Uibridge->Message(codec->Request, 0, 0, current.revision,
                                             current.rangefirst, current.rangelast, ""));
                    status = copied.status;
                    if (stored.text == "") {
                        note = "Session clipboard: empty; source kept unchanged.";
                    }
                } else {
                    exported := panecall(windows[active - 1], panes->ExportSelection, "", nil);
                    status = exported.status;
                    if (exported.rangefirst == exported.rangelast) {
                        note = "Session clipboard: no selection; previous value kept.";
                    }
                    if (status == Idecore->Ok && exported.rangefirst != exported.rangelast) {
                        stored = clipcall(clipboard, clips->Put, stored.generation, exported.transfer);
                        status = stored.status;
                        if (stored.status == clips->Ok && action == desk->CutSelection) {
                            cut := panecall(windows[active - 1], panes->CutSelection, "",
                                ref Uibridge->Message(codec->Request, 0, 0, exported.revision,
                                                     exported.rangefirst, exported.rangelast, ""));
                            status = cut.status;
                        }
                    }
                }
                if (status != 0) {
                    note = "Session clipboard: command rejected; no source edit. Requires a fresh selection, valid Unicode and at most 64 KiB.";
                } else if (note == "") {
                    verb := "Copied";
                    if (action == desk->CutSelection) {
                        verb = "Cut";
                    }
                    if (action == desk->PasteSelection) {
                        verb = "Pasted";
                    }
                    note = "Session clipboard: " + verb + " " + string stored.bytes + " bytes inside this IDE.";
                }
                panecall(windows[1], panes->SetText, note, nil);
            }
        } else if (action == desk->CancelClose) {
            closing = 0;
        } else if (action == desk->CloseEditor) {
            # A title button may target an inactive window. Change focus only
            # after layout admission and preserve every other document owner.
            if (view.focus != active) {
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                active = view.focus;
            }
            current := panecall(windows[active - 1], panes->Snapshot, "", nil);
            if (current.kind != panes->Editor && current.kind != panes->Vector && current.kind != panes->Debugger) {
                panecall(windows[1], panes->SetText, "Help and build console stay open. Select an editor to close.", nil);
            } else if (current.revision != current.saved && closing == 0 && !approved) {
                closing = active;
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                request.operation = desk->Tool;
                request.window = desk->CloseEditor;
                view = deskcall(layout, request);
            } else {
                if (current.kind == panes->Editor || current.kind == panes->Vector) {
                    before := mdicall(membership, mdirevision, mdi->Snapshot, 0, 0);
                    oldcontainer := before.containers[active - 1];
                    removed := mdicall(membership, mdirevision, mdi->Remove, active, 0);
                    if (removed.status != mdi->Ok) {
                        raise "fail: MDI document retirement";
                    }
                    mdirevision = removed.revision;
                    if (oldcontainer != 0) {
                        remaining := 0;
                        for (member := 0; member < mdi->Limit; member++) {
                            remaining += removed.containers[member] == oldcontainer;
                        }
                        if (!remaining) {
                            dissolved := mdigroupcall(membership, mdirevision, mdi->Dissolve, 0, oldcontainer, 0);
                            mdirevision = dissolved.revision;
                        }
                    }
                }
                # Consume the terminal acknowledgement before reusing the slot.
                panecall(windows[active - 1], panes->Shutdown, "", nil);
                windows[active - 1] = nil;
                for (panel := 0; panel < len debugpanes; panel++) {
                    if (debugpanes[panel] == active) {
                        debugpanes[panel] = 0;
                    }
                }
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
            if (current.kind == panes->Editor || current.kind == panes->Vector) {
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
        } else if (action == desk->GroupEditor || action == desk->DetachEditor) {
            current := panecall(windows[active - 1], panes->Snapshot, "", nil);
            if (current.kind == panes->Editor || current.kind == panes->Vector) {
                family := mdi->Source;
                if (current.kind == panes->Vector) {
                    family = mdi->Graphics;
                }
                before := mdicall(membership, mdirevision, mdi->Snapshot, 0, 0);
                target := 0;
                if (action == desk->GroupEditor) {
                    if (!shift) {
                        for (group := 0; group < mdi->Limit; group++) {
                            if (before.tools[group] == family) {
                                target = group + 1;
                                break;
                            }
                        }
                    }
                    if (target == 0) {
                        for (group := 0; group < mdi->Limit; group++) {
                            if (before.tools[group] == 0) {
                                target = group + 1;
                                break;
                            }
                        }
                        if (target != 0) {
                            made := mdigroupcall(membership, mdirevision, mdi->Create, 0, target, family);
                            if (made.status != mdi->Ok) {
                                raise "fail: MDI container creation";
                            }
                            mdirevision = made.revision;
                        }
                    }
                }
                if (action == desk->DetachEditor || target != 0) {
                    operation := mdi->Join;
                    if (target == 0) {
                        operation = mdi->Leave;
                    }
                    changed := mdigroupcall(membership, mdirevision, operation, active, target, 0);
                    if (changed.status != mdi->Ok) {
                        raise "fail: MDI membership change";
                    }
                    mdirevision = changed.revision;
                    request.operation = desk->SetContainer;
                    request.window = active;
                    request.phase = target;
                    request.x = family;
                    view = deskcall(layout, request);
                    if (view.status != desk->Ok) {
                        raise "fail: MDI presentation change";
                    }
                    request.operation = desk->Select;
                    request.window = active;
                    view = deskcall(layout, request);
                    old := before.containers[active - 1];
                    if (old != 0 && old != target) {
                        used := 0;
                        for (document := 0; document < mdi->Limit; document++) {
                            used += changed.containers[document] == old;
                        }
                        if (!used) {
                            dissolved := mdigroupcall(membership, mdirevision, mdi->Dissolve, 0, old, 0);
                            mdirevision = dissolved.revision;
                        }
                    }
                } else {
                    panecall(windows[1], panes->SetText, "MDI container limit reached; document kept unchanged.", nil);
                }
            } else {
                panecall(windows[1], panes->SetText, "MDI/SDI applies to source and graphics documents; debugger panels stay independent.", nil);
            }
        } else if (action == desk->Debugger) {
            replies := chan of Idedebug->Reply;
            session <-= Idedebug->Request(1, 1, debugger->Snapshot, replies);
            snapshot := <-replies;
            titles := array[] of {"Debug: source", "Debug: stack", "Debug: data"};
            bodies := array[] of {snapshot.source, snapshot.stack, snapshot.data};
            for (panel := 0; panel < len debugpanes; panel++) {
                if (debugpanes[panel] != 0 && windows[debugpanes[panel] - 1] != nil) {
                    continue;
                }
                for (slot := 0; slot < len windows; slot++) {
                    if (windows[slot] == nil) {
                        owner := panes->start(panes->Debugger, titles[panel], bodies[panel]);
                        if (owner != nil) {
                            windows[slot] = owner;
                            debugpanes[panel] = slot + 1;
                        }
                        break;
                    }
                }
            }
            if (debugpanes[0] != 0) {
                panecall(windows[active - 1], panes->Input, "", ref Uibridge->Message(codec->Focus, 0, 0, 0, 0, 0, ""));
                active = debugpanes[0];
                request.operation = desk->Select;
                request.window = active;
                view = deskcall(layout, request);
            }
            panecall(windows[1], panes->SetText,
                "Debugger panels share this desktop. No target attached; run controls remain unavailable.", nil);
        } else if (action == desk->NewEditor || action == desk->VectorEditor) {
            created := 0;
            for (index = 0; index < len windows; index++) {
                if (windows[index] == nil) {
                    kind := panes->Editor;
                    if (action == desk->VectorEditor) {
                        kind = panes->Vector;
                    }
                    owner := panes->startnotified(kind, "", "", notices, index + 1);
                    if (owner == nil) {
                        panecall(windows[1], panes->SetText, "Cannot start an editor owner.", nil);
                        break;
                    }
                    family := mdi->Source;
                    if (kind == panes->Vector) {
                        family = mdi->Graphics;
                    }
                    registered := mdicall(membership, mdirevision, mdi->Register, index + 1, family);
                    if (registered.status != mdi->Ok) {
                        raise "fail: new MDI registration";
                    }
                    mdirevision = registered.revision;
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
    clipcall(clipboard, clips->Shutdown, 0, "");
    request.operation = desk->Shutdown;
    stopped := deskcall(layout, request);
    if (stopped.status != desk->Ok) {
        raise "fail: desktop layout shutdown";
    }
    mdistopped := mdicall(membership, mdirevision, mdi->Shutdown, 0, 0);
    if (mdistopped.status != mdi->Ok) {
        raise "fail: MDI registry shutdown";
    }
    debugreply := chan of Idedebug->Reply;
    session <-= Idedebug->Request(1, 1, debugger->Shutdown, debugreply);
    debugstopped := <-debugreply;
    if (debugstopped.status != debugger->Ok) {
        raise "fail: debugger session shutdown";
    }
    host->detach();
    sys->print("PASS MovieClip desktop: independent panes and acknowledged shutdown\n");
}
