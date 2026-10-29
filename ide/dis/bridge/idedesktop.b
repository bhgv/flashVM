implement Idedesktop;
include "idedesktop.m";

Window: adt
{
    title: string; # Immutable caption value.
    body: string; # Copied presentation, not source authority.
    caret: int; # Body-local display caret.
    dx: int; # Persistent horizontal offset from the responsive grid.
    dy: int; # Persistent vertical offset from the responsive grid.
};


# Compute a window's fixed grid geometry in table-local pixels. index is the
# bounded zero-based slot; dimensions are copied and contain no native handles.
geometry(index, width: int): (int, int, int, int)
{
    columns := 1;
    if (width >= 900) {
        columns = 2;
    }
    wide := (width - 216) / columns;
    if (wide < 24) {
        wide = 24;
    }
    return (96 + (index % columns) * (wide + 16), 72 + (index / columns) * 288, wide, 252);
}


# Return bounded table-local geometry for a borrowed window and canvas width.
# Offsets survive content updates; resizing clamps presentation without losing
# the stored position. No UI object or document ownership is acquired.
position(index, width: int, window: ref Window): (int, int, int, int)
{
    (x, y, wide, high) := geometry(index, width);
    x += window.dx;
    y += window.dy;
    if (x < 84) {
        x = 84;
    }
    if (x > width - 128) {
        x = width - 128;
    }
    if (y < 48) {
        y = 48;
    }
    if (y > 2600) {
        y = 2600;
    }
    return (x, y, wide, high);
}


# Map a bounded paint/hit ordinal to a stable slot; focused windows come last.
# Missing slots are skipped by callers. This keeps overlapping hit order equal
# to actual MovieClip paint order without changing a document's identity.
stackslot(order, focus: int): int
{
    if (focus < 1 || focus > MaxWindows) {
        return order;
    }
    if (order == MaxWindows - 1) {
        return focus - 1;
    }
    if (order >= focus - 1) {
        return order + 1;
    }
    return order;
}


# Build fresh scene descriptors. windows is borrowed only during this call;
# the returned array/Items are new, and all text values are immutable Dis strings.
scene(windows: array of ref Window, width, height, scroll, focus, drawer: int): array of ref Item
{
    items := array[52] of ref Item;
    used := 0;
    items[used++] = ref Item(10, 0, 0, 0, 0, width, height, 2, "", -1);
    for (order := 0; order < len windows; order++) {
        index := stackslot(order, focus);
        window := windows[index];
        if (window == nil) {
            continue;
        }
        (x, y, wide, high) := position(index, width, window);
        style := 0;
        if (focus == index + 1) {
            style = 1;
        }
        caption := window.title + "\n" + window.body;
        caret := -1;
        if (focus == index + 1 && window.caret >= 0) {
            caret = len window.title + 1 + window.caret;
        }
        items[used++] = ref Item(index + 1, 1, 0, x, y, wide, high, style, caption, caret);
        if (index != 1 && index != 2) {
            items[used++] = ref Item(30 + index, 1, 0, x + wide - 30, y, 30, 24, 4, "x", -1);
        }
    }
    items[used++] = ref Item(11, 2, 0, 0, 0, 80, height, 2, "", -1);
    items[used++] = ref Item(12, 3, 0, 0, 0, 104, height, 2, "", -1);
    for (index := 0; index < len windows; index++) {
        if (windows[index] != nil) {
            items[used++] = ref Item(20 + index, 2, index + 1, 6, 72 + index * 64, 68, 54,
                                    index + 1 == focus, windows[index].title, -1);
        }
    }
    names := array[] of {"", "Open", "Save", "Build", "New", "Debug", "Help", "Quit", "Close"};
    for (index = 1; index <= CloseEditor; index++) {
        style := 4;
        if (drawer == index || index == Help) {
            style = 3;
        }
        items[used++] = ref Item(40 + index, 3, 0, 8, 56 + (index - 1) * 64, 88, 54, style, names[index], -1);
    }
    if (drawer != 0) {
        descriptions := array[] of {"", "Open a UTF-8 source file in the active editor. Unsaved edits are protected.",
            "Save the active editor. Choose a path for an untitled document.",
            "Compile the current revision using the library service. F8 selects a source diagnostic.",
            "Create another independent editor. Existing documents remain open.",
            "No debugger is attached. This button does not fabricate a debug session.",
            "F1 Help. F7 Build. Ctrl+S Save. Keyboard edits the focused editor. Drag EMPTY table space to scroll. Left thumbnails jump to windows. Escape closes a scroll.",
            "Close the desktop. The action button explicitly discards unsaved documents.",
            "Close this editor only. Unsaved changes will be discarded. Escape or Cancel keeps the document open."};
        x := width - 424;
        if (x < 84) {
            x = 84;
        }
        wide := width - 112 - x;
        if (wide < 24) {
            wide = 24;
        }
        items[used++] = ref Item(55, 4, 0, x, 72, wide, 340, 0, names[drawer] + "\n" + descriptions[drawer], -1);
        labels := array[] of {"", "Choose file", "Save editor", "Build editor", "New editor", "Not attached", "Read Help", "Discard and close", "Discard this editor"};
        label := labels[drawer];
        if (drawer == Debugger) {
            label = "Not attached";
        }
        items[used++] = ref Item(56, 4, 0, x + 8, 320, wide - 16, 36, 3, label, -1);
        if (drawer == CloseEditor) {
            items[used++] = ref Item(58, 4, 0, x + 8, 364, wide - 16, 36, 4, "Cancel", -1);
        }
    }
    items[used++] = ref Item(63, 0, 0, 0, 0, width, 48, 2, "FlashVM IDE     |     MovieClip desk     |     F1 Help", -1);
    return items[:used];
}


# Own layout and gesture state. Every request/reply is a typed value. Child
# windows own their documents separately; source edits never occur in this owner.
serve(requests: chan of Request)
{
    windows := array[MaxWindows] of ref Window;
    width := 960;
    height := 640;
    scroll := 0;
    focus := 1;
    drawer := 0;
    ticket := 1;
    dragging := 0;
    moving := 0;
    windowx := 0;
    windowy := 0;
    pressed := 0;
    startx := 0;
    starty := 0;
    origin := 0;
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Invalid;
        action := 0;
        changed := 0;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Shutdown) {
                status = Ok;
                stop = 1;
            } else if (request.operation == Cancel) {
                dragging = 0;
                moving = 0;
                pressed = 0;
                status = Ok;
            } else if (request.operation == View) {
                status = Ok;
            } else if (request.operation == Update && request.window >= 1 && request.window <= MaxWindows &&
                       len request.title <= 32 && len request.body <= 200 && request.caret >= -1 &&
                       request.caret <= len request.body) {
                old := windows[request.window - 1];
                changed = old == nil || old.title != request.title || old.body != request.body || old.caret != request.caret;
                dx := 0;
                dy := 0;
                if (old != nil) {
                    dx = old.dx;
                    dy = old.dy;
                }
                windows[request.window - 1] = ref Window(request.title, request.body, request.caret, dx, dy);
                status = Ok;
            } else if (request.operation == Remove && request.window >= 1 && request.window <= MaxWindows &&
                       request.window != 2 && request.window != 3 && windows[request.window - 1] != nil) {
                windows[request.window - 1] = nil;
                dragging = 0;
                moving = 0;
                pressed = 0;
                drawer = 0;
                if (focus == request.window) {
                    focus = 0;
                    for (slot := 0; slot < len windows; slot++) {
                        if (windows[slot] != nil) {
                            focus = slot + 1;
                            break;
                        }
                    }
                }
                changed = 1;
                status = Ok;
            } else if (request.operation == Resize && request.x >= 240 && request.x <= 2048 &&
                       request.y >= 160 && request.y <= 2048) {
                dragging = 0;
                moving = 0;
                pressed = 0;
                width = request.x;
                height = request.y;
                changed = 1;
                status = Ok;
            } else if (request.operation == Select && request.window >= 1 && request.window <= MaxWindows && windows[request.window - 1] != nil) {
                dragging = 0;
                moving = 0;
                pressed = 0;
                focus = request.window;
                (unused, top, unusedwide, unusedheight) := position(focus - 1, width, windows[focus - 1]);
                scroll = top - 72;
                changed = 1;
                status = Ok;
            } else if (request.operation == Tool && request.window >= 0 && request.window <= CloseEditor) {
                dragging = 0;
                moving = 0;
                pressed = 0;
                drawer = request.window;
                changed = 1;
                status = Ok;
            } else if (request.operation == Pointer || request.operation == Scroll) {
                if (request.ticket != ticket) {
                    status = Conflict;
                    dragging = 0;
                    moving = 0;
                    pressed = 0;
                } else if (request.operation == Scroll && request.x >= -2048 && request.x <= 2048) {
                    dragging = 0;
                    moving = 0;
                    pressed = 0;
                    scroll += request.x;
                    changed = 1;
                    status = Ok;
                } else if (request.operation == Pointer && request.x >= 0 && request.x < width &&
                           request.y >= 0 && request.y < height && request.phase >= 1 && request.phase <= 3) {
                    x := request.x;
                    y := request.y;
                    status = Ok;
                    drawerleft := width - 424;
                    if (drawerleft < 84) {
                        drawerleft = 84;
                    }
                    if (request.phase == 1) {
                        dragging = 0;
                        moving = 0;
                        pressed = 0;
                        startx = x;
                        starty = y;
                        origin = scroll;
                        if (drawer == CloseEditor && x >= drawerleft + 8 && x < width - 120 && y >= 364 && y < 400) {
                            action = CancelClose;
                            drawer = 0;
                        } else if (drawer != 0 && x >= drawerleft + 8 && x < width - 120 && y >= 320 && y < 356) {
                            if (drawer != Debugger) {
                                action = drawer;
                                drawer = 0;
                            }
                        } else if (drawer != 0 && x >= drawerleft && x < width - 112 && y >= 72 && y < 412) {
                            # The scroll owns this area; never focus or drag the covered window.
                        } else if (x >= width - 104 && y >= 56) {
                            tool := (y - 56) / 64 + 1;
                            if (tool <= CloseEditor) {
                                if (drawer == tool) {
                                    drawer = 0;
                                } else {
                                    drawer = tool;
                                }
                            }
                        } else if (x < 80 && y >= 72) {
                            slot := (y - 72) / 64;
                            if (slot < len windows && windows[slot] != nil) {
                                focus = slot + 1;
                                (unused, top, unusedwide, unusedheight) := position(slot, width, windows[slot]);
                                scroll = top - 72;
                                action = Focus;
                            }
                        } else if (x >= 80 && x < width - 104 && y >= 48) {
                            for (order := 0; order < len windows; order++) {
                                index := stackslot(order, focus);
                                if (windows[index] == nil) {
                                    continue;
                                }
                                (left, top, wide, high) := position(index, width, windows[index]);
                                top -= scroll;
                                if (x >= left && x < left + wide && y >= top && y < top + high) {
                                    pressed = index + 1;
                                    moving = 0;
                                    if (y < top + 24) {
                                        if (x >= left + wide - 30 && index != 1 && index != 2) {
                                            pressed = -(index + 1);
                                        } else {
                                            moving = index + 1;
                                            windowx = windows[index].dx;
                                            windowy = windows[index].dy;
                                        }
                                    }
                                }
                            }
                            dragging = pressed == 0 && drawer == 0;
                        }
                        changed = 1;
                    } else if (request.phase == 2) {
                        if (dragging) {
                            scroll = origin + starty - y;
                        }
                        if (moving != 0) {
                            window := windows[moving - 1];
                            window.dx = windowx + x - startx;
                            window.dy = windowy + y - starty;
                            if (window.dx < -2048) {
                                window.dx = -2048;
                            }
                            if (window.dx > 2048) {
                                window.dx = 2048;
                            }
                            if (window.dy < -2600) {
                                window.dy = -2600;
                            }
                            if (window.dy > 2600) {
                                window.dy = 2600;
                            }
                        }
                        if (x - startx > 8 || startx - x > 8 || y - starty > 8 || starty - y > 8) {
                            pressed = 0;
                        }
                        changed = 1;
                    } else {
                        if (moving == 0 && (x - startx > 8 || startx - x > 8 ||
                                            y - starty > 8 || starty - y > 8)) {
                            pressed = 0;
                        }
                        if (pressed < 0) {
                            (left, top, wide, high) := position(-pressed - 1, width, windows[-pressed - 1]);
                            top -= scroll;
                            if (x < left + wide - 30 || x >= left + wide || y < top || y >= top + 24) {
                                pressed = 0;
                            }
                        }
                        if (pressed < 0) {
                            focus = -pressed;
                            action = CloseEditor;
                        } else if (moving != 0 || pressed != 0) {
                            focus = pressed;
                            if (moving != 0) {
                                focus = moving;
                            }
                            action = Focus;
                        }
                        pressed = 0;
                        dragging = 0;
                        moving = 0;
                        changed = 1;
                    }
                }
            }
        }
        if (status != Ok && (request.operation == Pointer || request.operation == Scroll)) {
            dragging = 0;
            moving = 0;
            pressed = 0;
        }
        maximum := 72 + 9 * 288 - height;
        if (scroll < 0) {
            scroll = 0;
        }
        if (scroll > maximum) {
            scroll = maximum;
        }
        if (changed) {
            if (ticket == 2147483647) {
                raise "fail: desktop view ticket exhausted";
            }
            ticket++;
        }
        request.reply <-= Reply(request.id, status, ticket, scroll, focus, drawer, action,
                                scene(windows, width, height, scroll, focus, drawer));
        if (stop) {
            return;
        }
    }
}


# Start one isolated geometry/gesture process. No renderer, file or document is
# acquired; only the caller owns the returned terminal request channel.
start(): chan of Request
{
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}
