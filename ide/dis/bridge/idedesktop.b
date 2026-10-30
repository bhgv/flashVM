implement Idedesktop;
include "idedesktop.m";


# Copied presentation state; document authority stays with its separate pane owner.
Window: adt
{
    title: string; # Immutable caption value.
    body: string; # Copied presentation, not source authority.
    caret: int; # Body-local display caret.
    anchor: int; # Clipped body-local selection anchor.
    maximized: int; # Presentation-only max/normal state; normal offsets remain saved.
    maxwide: int; # Max extent in unscaled table pixels.
    maxhigh: int; # Max extent in unscaled table pixels.
    maxtop: int; # Max origin at the current visible table top.
    line: int; # Source-authoritative logical viewport line; zero for system panes.
    hidden: int; # Presentation visibility; its document/help owner remains alive.
    zoom: int; # Bounded percent of responsive grid dimensions, 50..180.
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
    if (window.maximized) {
        return (84, window.maxtop, window.maxwide, window.maxhigh);
    }
    wide = wide * window.zoom / 100;
    high = high * window.zoom / 100;
    x += window.dx;
    y += window.dy;
    if (x < -2048) {
        x = -2048;
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


# Project immutable table geometry into current canvas coordinates. Scroll is
# applied by callers; source identity and normal window geometry are unchanged.
project(index, width: int, window: ref Window, zoom, pan: int): (int, int, int, int)
{
    (x, y, wide, high) := position(index, width, window);
    return (84 + (x - 84) * zoom / 100 + pan, 48 + (y - 48) * zoom / 100,
            wide * zoom / 100, high * zoom / 100);
}


# Toggle max/normal without overwriting the saved ordinary position or size.
# Inputs are bounded copied canvas values; no document ownership is acquired.
maximize(window: ref Window, width, height, scroll, zoom: int)
{
    window.maximized = !window.maximized;
    window.maxwide = (width - 196) * 100 / zoom;
    window.maxhigh = (height - 60) * 100 / zoom;
    window.maxtop = 48 + scroll * 100 / zoom;
}


# Apply a bounded absolute drag proposal in table pixels. A maximized window
# preserves normal offsets; the caller borrows its presentation-only record.
move(window: ref Window, dx, dy: int)
{
    if (window.maximized) {
        return;
    }
    if (dx < -2048) {
        dx = -2048;
    }
    if (dx > 2048) {
        dx = 2048;
    }
    if (dy < -2600) {
        dy = -2600;
    }
    if (dy > 2600) {
        dy = 2600;
    }
    window.dx = dx;
    window.dy = dy;
}


# Reveal a borrowed logical pane by moving only the viewport. Return copied
# scroll/zoom/pan values; normal window offsets and source owners stay unchanged.
# Oversized panes reduce the whole-table zoom only enough to fit both dimensions.
restore(window: ref Window, index, width, height, scroll, zoom, pan: int): (int, int, int)
{
    window.hidden = 0;
    (originx, originy, wide, high) := position(index, width, window);
    # Keep one spare pixel for fractional transforms and round exposed bounds
    # outward; integer truncation must not hide a sliver beneath a rail.
    fit := (width - 195) * 100 / wide;
    vertical := (height - 54) * 100 / high;
    if (vertical < fit) {
        fit = vertical;
    }
    if (zoom > fit) {
        zoom = fit;
    }
    if (zoom < 10) {
        zoom = 10;
    }
    horizontal := 8400 + (originx - 84) * zoom + pan * 100;
    left := horizontal / 100;
    if (horizontal < 0) {
        left = (horizontal - 99) / 100;
    }
    if (left < 85) {
        delta := 85 - left;
        pan += delta;
        horizontal += delta * 100;
    }
    right := (horizontal + wide * zoom + 99) / 100;
    if (right > width - 109) {
        pan -= right - (width - 109);
    }
    verticalposition := 4800 + (originy - 48) * zoom - scroll * 100;
    top := verticalposition / 100;
    if (verticalposition < 0) {
        top = (verticalposition - 99) / 100;
    }
    if (top < 48) {
        targettop := 72;
        if ((high * zoom + 99) / 100 > height - 77) {
            targettop = 48;
        }
        scroll += top - targettop;
    } else {
        bottom := (verticalposition + high * zoom + 99) / 100;
        if (bottom > height - 5) {
            scroll += bottom - (height - 5);
        }
    }
    return (scroll, zoom, pan);
}


# Dock only after the right edge leaves at most20 physical pixels beyond the
# left rail. Keep pre-drag normal offsets for restoration. Return one if docked;
# no source owner is closed, moved or blocked by this presentation decision.
dock(window: ref Window, index, width, zoom, pan, dx, dy: int): int
{
    (left, unused, wide, unusedhigh) := project(index, width, window, zoom, pan);
    if (window.maximized || left + wide > 100) {
        return 0;
    }
    window.hidden = 1;
    window.dx = dx;
    window.dy = dy;
    return 1;
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


# Admit a text-body identity from copied pixels in the same paint/hit order.
# Borrowed windows contain presentation only. Return zero for rails, headers,
# opaque scrolls and no hit; source kind/revision/ranges stay with the pane owner.
texttarget(windows: array of ref Window, width, height, scroll, focus, drawer, x, y, zoom, pan: int): int
{
    if (x < 80 || x >= width - 104 || y < 48 || y >= height) {
        return 0;
    }
    left := width - 424;
    if (left < 84) {
        left = 84;
    }
    if (drawer != 0 && x >= left && x < width - 112 && y >= 72 && y < 412) {
        return 0;
    }
    target := 0;
    for (order := 0; order < len windows; order++) {
        index := stackslot(order, focus);
        if (windows[index] == nil || windows[index].hidden) {
            continue;
        }
        (x0, y0, wide, high) := project(index, width, windows[index], zoom, pan);
        y0 -= scroll;
        if (x >= x0 && x < x0 + wide && y >= y0 && y < y0 + high) {
            target = 0;
            if (y >= y0 + 24 * zoom / 100) {
                target = index + 1;
            }
        }
    }
    return target;
}


# Build fresh scene descriptors. windows is borrowed only during this call;
# the returned array/Items are new, and all text values are immutable Dis strings.
scene(windows: array of ref Window, width, height, scroll, focus, drawer, moving, zooming: int, thumbnails: array of int): array of ref Item
{
    items := array[52] of ref Item;
    used := 0;
    items[used++] = ref Item(10, 0, 0, 0, 0, width, height, 2, "", -1, -1);
    for (order := 0; order < len windows; order++) {
        index := stackslot(order, focus);
        window := windows[index];
        if (window == nil) {
            continue;
        }
        (x, y, wide, high) := position(index, width, window);
        if (window.hidden) {
            x = 65536;
            y = 65536;
        }
        style := 0;
        if (focus == index + 1) {
            style = 1;
        }
        if (moving == index + 1 || (zooming && !window.hidden)) {
            style = 6; # Transient opacity without an active outline.
            if (focus == index + 1) {
                style = 5; # Only the active transient window has a blue outline.
            }
        }
        caption := window.title + "\n" + window.body;
        caret := -1;
        anchor := -1;
        if (focus == index + 1 && window.caret >= 0) {
            caret = len window.title + 1 + window.caret;
            if (window.anchor >= 0) {
                anchor = len window.title + 1 + window.anchor;
            }
        }
        items[used++] = ref Item(index + 1, 1, 0, x, y, wide, high, style + window.line * 8, caption, caret, anchor);
        if (index != 1 && index != 2 && !window.hidden) {
            items[used++] = ref Item(30 + index, 1, 0, x + wide - 30, y, 30, 24, 4, "x", -1, -1);
        }
    }
    items[used++] = ref Item(11, 2, 0, 0, 0, 80, height, 2, "", -1, -1);
    items[used++] = ref Item(12, 3, 0, 0, 0, 104, height, 2, "", -1, -1);
    for (rank := 0; rank < len thumbnails; rank++) {
        index := thumbnails[rank];
        if (windows[index] != nil) {
            items[used++] = ref Item(20 + index, 2, index + 1, 6, 72 + rank * 64, 68, 54,
                                    index + 1 == focus, windows[index].title, -1, -1);
        }
    }
    names := array[] of {"", "Open", "Save", "Build", "New", "Debug", "Help", "Quit", "Close", "Clip", "All", "Max/N", "Draw"};
    for (index := 1; index <= MaxNorm + 1; index++) {
        style := 4;
        if (drawer == index || index == Help) {
            style = 3;
        }
        items[used++] = ref Item(40 + index, 3, 0, 30, 56 + (index - 1) * 32, 44, 27, style, names[index], -1, -1);
    }
    if (drawer != 0) {
        descriptions := array[] of {"", "Open a UTF-8 source file in the active editor. Unsaved edits are protected.",
            "Save the active editor. Choose a path for an untitled document.",
            "Compile the current revision using the library service. F8 selects a source diagnostic.",
            "Create another independent editor. Existing documents remain open.",
            "Open source/stack/data debugger panels on this desktop. No runtime target is attached.",
            "F1 Help. F7 Build. Ctrl+S Save. Keyboard edits the focused editor. Drag EMPTY table space to scroll. Left thumbnails jump to windows. Escape closes a scroll.",
            "Close the desktop. The action button explicitly discards unsaved documents.",
            "Close this editor only. Unsaved changes will be discarded. Escape or Cancel keeps the document open.",
            "Session clipboard between editor owners. Ctrl+C Copy, Ctrl+X Cut, Ctrl+V Paste. This is not the operating-system clipboard.", "Fit all open windows without changing table positions.", "Toggle active window between maximized and normal geometry."};
        x := width - 424;
        if (x < 84) {
            x = 84;
        }
        wide := width - 112 - x;
        if (wide < 24) {
            wide = 24;
        }
        items[used++] = ref Item(55, 4, 0, x, 72, wide, 340, 0, names[drawer] + "\n" + descriptions[drawer], -1, -1);
        labels := array[] of {"", "Choose file", "Save editor", "Build editor", "New editor", "Not attached", "Read Help", "Discard and close", "Discard this editor", "Copy", "Show all", "Max/Norm"};
        label := labels[drawer];
        if (drawer == Debugger) {
            label = "Open panels";
        }
        if (drawer == ClipboardTool) {
            button := (wide - 32) / 3;
            if (button < 1) {
                button = 1;
            }
            items[used++] = ref Item(56, 4, 0, x + 8, 320, button, 36, 3, "Copy", -1, -1);
            items[used++] = ref Item(58, 4, 0, x + 16 + button, 320, button, 36, 3, "Cut", -1, -1);
            items[used++] = ref Item(59, 4, 0, x + 24 + 2 * button, 320, button, 36, 3, "Paste", -1, -1);
        } else {
            items[used++] = ref Item(56, 4, 0, x + 8, 320, wide - 16, 36, 3, label, -1, -1);
        }
        if (drawer == CloseEditor) {
            items[used++] = ref Item(58, 4, 0, x + 8, 364, wide - 16, 36, 4, "Cancel", -1, -1);
        }
    }
    items[used++] = ref Item(63, 0, 0, 0, 0, width, 48, 2, "FlashVM IDE     |     MovieClip desk     |     F1 Help", -1, -1);
    return items[:used];
}


# Own layout and gesture state. Every request/reply is a typed value. Child
# windows own their documents separately; source edits never occur in this owner.
serve(requests: chan of Request)
{
    deskzoom := 100;
    zoomreference := 0; # Last exact viewport anchor's scale, zero until first table zoom.
    scrollreference := 0; # Physical scroll at that reference scale; avoids cumulative rounding.
    panreference := 0; # Signed physical pan at that reference scale.
    deskpan := 0; # Physical horizontal viewport translation; window coordinates stay persistent.
    reveal := 0; # Changes only for explicit thumbnail/Select viewport reveal proposals.
    zooming := 0; # Transient table-wide opacity, cleared on release/cancel.
    windows := array[MaxWindows] of ref Window;
    thumbnails := array[MaxWindows] of int;
    for (rank := 0; rank < len thumbnails; rank++) {
        thumbnails[rank] = rank;
    }
    thumbdrag := -1; # Rank being reordered; source slots and document owners never move.
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
                zooming = 0;
                dragging = 0;
                moving = 0;
                thumbdrag = -1;
                pressed = 0;
                status = Ok;
            } else if (request.operation == View) {
                status = Ok;
            } else if (request.operation == Update && request.window >= 1 && request.window <= MaxWindows &&
                       len request.title <= 32 && len request.body <= 200 && request.caret >= -1 &&
                       request.caret <= len request.body && request.anchor >= -1 && request.anchor <= len request.body) {
                old := windows[request.window - 1];
                changed = old == nil || old.title != request.title || old.body != request.body || old.caret != request.caret || old.anchor != request.anchor;
                hidden := 0;
                zoom := 100;
                dx := 0;
                dy := 0;
                if (old != nil) {
                    hidden = old.hidden;
                    zoom = old.zoom;
                    dx = old.dx;
                    dy = old.dy;
                }
                windows[request.window - 1] = ref Window(request.title, request.body, request.caret, request.anchor, 0, 0, 0, 0, request.line, hidden, zoom, dx, dy);
                if (old != nil) {
                    current := windows[request.window - 1];
                    current.maximized = old.maximized;
                    current.maxwide = old.maxwide;
                    current.maxhigh = old.maxhigh;
                    current.maxtop = old.maxtop;
                }
                status = Ok;
            } else if (request.operation == Remove && request.window >= 1 && request.window <= MaxWindows &&
                       request.window != 2 && request.window != 3 && windows[request.window - 1] != nil) {
                windows[request.window - 1] = nil;
                dragging = 0;
                moving = 0;
                thumbdrag = -1;
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
                thumbdrag = -1;
                pressed = 0;
                width = request.x;
                height = request.y;
                for (slot := 0; slot < len windows; slot++) {
                    if (windows[slot] != nil && windows[slot].maximized) {
                        windows[slot].maximized = 0;
                        maximize(windows[slot], width, height, scroll, deskzoom);
                    }
                }
                changed = 1;
                status = Ok;
            } else if (request.operation == TextHit) {
                status = Conflict;
                if (request.ticket == ticket && request.window >= 1 && request.window <= MaxWindows &&
                    texttarget(windows, width, height, scroll, focus, drawer, request.x, request.y, deskzoom, deskpan) == request.window) {
                    status = Ok;
                }
            } else if (request.operation == Activate && request.window >= 1 && request.window <= MaxWindows && windows[request.window - 1] != nil) {
                dragging = 0;
                moving = 0;
                thumbdrag = -1;
                pressed = 0;
                focus = request.window;
                changed = 1;
                status = Ok;
            } else if (request.operation == Select && request.window >= 1 && request.window <= MaxWindows && windows[request.window - 1] != nil) {
                dragging = 0;
                moving = 0;
                thumbdrag = -1;
                pressed = 0;
                focus = request.window;
                (scroll, deskzoom, deskpan) = restore(windows[focus - 1], focus - 1, width, height, scroll, deskzoom, deskpan);
                if (reveal == 2147483647) {
                    raise "fail: desktop reveal epoch exhausted";
                }
                reveal++;
                changed = 1;
                status = Ok;
            } else if (request.operation == Tool && request.window >= 0 && request.window <= MaxNorm) {
                dragging = 0;
                moving = 0;
                thumbdrag = -1;
                pressed = 0;
                drawer = request.window;
                changed = 1;
                status = Ok;
            } else if (request.operation == Pointer || request.operation == Scroll) {
                if (request.ticket != ticket) {
                    status = Conflict;
                    dragging = 0;
                    moving = 0;
                    thumbdrag = -1;
                    pressed = 0;
                } else if (request.operation == Scroll && request.x >= -2048 && request.x <= 2048) {
                    dragging = 0;
                    moving = 0;
                    thumbdrag = -1;
                    pressed = 0;
                    scroll += request.x;
                    changed = 1;
                    status = Ok;
                } else if (request.operation == Pointer && request.x >= 0 && request.x < width &&
                           request.y >= 0 && request.y < height && request.phase >= 1 && request.phase <= 15 && request.phase != 7 && request.phase != 8 && request.phase != 12) {
                    x := request.x;
                    y := request.y;
                    status = Ok;
                    phase := request.phase;
                    fingers := 1;
                    if (phase >= 9) {
                        fingers = phase / 4;
                        phase %= 4;
                    }
                    drawerleft := width - 424;
                    if (drawerleft < 84) {
                        drawerleft = 84;
                    }
                    if (phase == 4 || phase == 5 || phase == 6) {
                        moving = 0;
                        thumbdrag = -1;
                        dragging = 0;
                        pressed = 0;
                        target := 0;
                        obscured := drawer != 0 && x >= drawerleft && x < width - 112 && y >= 72 && y < 412;
                        for (order := 0; order < len windows; order++) {
                            index := stackslot(order, focus);
                            if (obscured || windows[index] == nil || windows[index].hidden) {
                                continue;
                            }
                            (left, top, wide, high) := project(index, width, windows[index], deskzoom, deskpan);
                            top -= scroll;
                            if (x >= left && x < left + wide && y >= top && y < top + high) {
                                target = 0;
                                if (y < top + 24 * deskzoom / 100) {
                                    target = index + 1;
                                }
                            }
                        }
                        if (phase != 6 && !obscured && target == 0 && x >= 80 && x < width - 104 && y >= 48) {
                            if (zoomreference == 0 || scroll != scrollreference * deskzoom / zoomreference ||
                                deskpan != panreference * deskzoom / zoomreference) {
                                zoomreference = deskzoom;
                                scrollreference = scroll;
                                panreference = deskpan;
                            }
                            if (phase == 4 && deskzoom <= 170) {
                                deskzoom += 10;
                            } else if (phase == 5 && deskzoom >= 20) {
                                deskzoom -= 10;
                            }
                            # Keep the logical viewport anchor at the same point.
                            # Scroll/pan are physical translations, so they scale
                            # with the parent; window coordinates remain untouched.
                            scroll = scrollreference * deskzoom / zoomreference;
                            deskpan = panreference * deskzoom / zoomreference;
                            zooming = 1;
                            changed = 1;
                        }
                        if (target != 0 && phase == 6) {
                            deskpan = 0;
                            maximize(windows[target - 1], width, height, scroll, deskzoom);
                            focus = target;
                            action = Focus;
                            changed = 1;
                        } else if (target != 0) {
                            zoom := windows[target - 1].zoom;
                            if (phase == 4) {
                                zoom += 10;
                            } else {
                                zoom -= 10;
                            }
                            if (zoom >= 50 && zoom <= 180) {
                                windows[target - 1].zoom = zoom;
                                moving = target;
                                windowx = windows[target - 1].dx;
                                windowy = windows[target - 1].dy;
                                startx = x;
                                starty = y;
                                focus = target;
                                action = Focus;
                                changed = 1;
                            }
                        }
                    } else if (phase == 1) {
                        zooming = 0;
                        dragging = 0;
                        moving = 0;
                        thumbdrag = -1;
                        pressed = 0;
                        thumbdrag = -1;
                        startx = x;
                        starty = y;
                        origin = scroll;
                        if (drawer == ClipboardTool && y >= 320 && y < 356) {
                            wide := width - 112 - drawerleft;
                            button := (wide - 32) / 3;
                            if (button < 1) {
                                button = 1;
                            }
                            for (slot := 0; slot < 3; slot++) {
                                left := drawerleft + 8 + slot * (button + 8);
                                if (x >= left && x < left + button) {
                                    action = CopySelection + slot;
                                }
                            }
                            if (action != 0) {
                                drawer = 0;
                            }
                        } else if (drawer == CloseEditor && x >= drawerleft + 8 && x < width - 120 && y >= 364 && y < 400) {
                            action = CancelClose;
                            drawer = 0;
                        } else if (drawer != 0 && x >= drawerleft + 8 && x < width - 120 && y >= 320 && y < 356) {
                            action = drawer;
                            drawer = 0;
                        } else if (drawer != 0 && x >= drawerleft && x < width - 112 && y >= 72 && y < 412) {
                            # The scroll owns this area; never focus or drag the covered window.
                        } else if (x >= width - 104 && y >= 56) {
                            tool := (y - 56) / 32 + 1;
                            if (tool == MaxNorm + 1) {
                                drawer = 0;
                                action = VectorEditor;
                            } else if (tool == MaxNorm && focus >= 1 && windows[focus - 1] != nil) {
                                deskpan = 0;
                                maximize(windows[focus - 1], width, height, scroll, deskzoom);
                                drawer = 0;
                            } else if (tool == ShowAll) {
                                minimumleft := 84;
                                right := 85;
                                bottom := 49;
                                for (slot := 0; slot < len windows; slot++) {
                                    if (windows[slot] == nil) {
                                        continue;
                                    }
                                    windows[slot].hidden = 0;
                                    (left, top, wide, high) := position(slot, width, windows[slot]);
                                    if (left < minimumleft) {
                                        minimumleft = left;
                                    }
                                    if (left + wide > right) {
                                        right = left + wide;
                                    }
                                    if (top + high > bottom) {
                                        bottom = top + high;
                                    }
                                }
                                deskzoom = (width - 196) * 100 / (right - minimumleft);
                                vertical := (height - 60) * 100 / (bottom - 48);
                                if (vertical < deskzoom) {
                                    deskzoom = vertical;
                                }
                                if (deskzoom < 10) {
                                    deskzoom = 10;
                                }
                                if (deskzoom > 180) {
                                    deskzoom = 180;
                                }
                                scroll = 0;
                                deskpan = 85 - (84 + (minimumleft - 84) * deskzoom / 100);
                                drawer = 0;
                            } else if (tool == Help && windows[2] != nil) {
                                windows[2].hidden = !windows[2].hidden;
                                drawer = 0;
                                focus = 3;
                                if (windows[2].hidden) {
                                    focus = 1;
                                }
                                action = Focus;
                            } else if (tool <= MaxNorm) {
                                if (drawer == tool) {
                                    drawer = 0;
                                } else {
                                    drawer = tool;
                                }
                            }
                        } else if (x < 80 && y >= 72) {
                            rank := (y - 72) / 64;
                            slot := -1;
                            if (rank < len thumbnails) {
                                slot = thumbnails[rank];
                            }
                            if (slot >= 0 && windows[slot] != nil) {
                                thumbdrag = rank;
                                pressed = slot + 1;
                            }
                        } else if (x >= 80 && x < width - 104 && y >= 48) {
                            for (order := 0; order < len windows; order++) {
                                index := stackslot(order, focus);
                                if (fingers == 3 || windows[index] == nil || windows[index].hidden) {
                                    continue;
                                }
                                (left, top, wide, high) := project(index, width, windows[index], deskzoom, deskpan);
                                top -= scroll;
                                if (x >= left && x < left + wide && y >= top && y < top + high) {
                                    pressed = index + 1;
                                    moving = 0;
                                    thumbdrag = -1;
                                    if (y < top + 24 * deskzoom / 100 || fingers == 2) {
                                        if (fingers == 1 && x >= left + wide - 30 * deskzoom / 100 && index != 1 && index != 2) {
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
                            if (moving != 0) {
                                focus = moving;
                                action = Focus;
                            }
                        }
                        changed = 1;
                    } else if (phase == 2) {
                        if (thumbdrag >= 0) {
                            rank := (y - 72) / 64;
                            if (rank < 0) {
                                rank = 0;
                            }
                            if (rank >= len thumbnails) {
                                rank = len thumbnails - 1;
                            }
                            slot := thumbnails[thumbdrag];
                            if (x - startx > 32) {
                                (scroll, deskzoom, deskpan) = restore(windows[slot], slot, width, height, scroll, deskzoom, deskpan);
                                if (reveal == 2147483647) {
                                    raise "fail: desktop reveal epoch exhausted";
                                }
                                reveal++;
                                focus = slot + 1;
                                action = Focus;
                            }
                            while (thumbdrag < rank) {
                                thumbnails[thumbdrag] = thumbnails[thumbdrag + 1];
                                thumbdrag++;
                            }
                            while (thumbdrag > rank) {
                                thumbnails[thumbdrag] = thumbnails[thumbdrag - 1];
                                thumbdrag--;
                            }
                            thumbnails[rank] = slot;
                        }
                        if (dragging) {
                            scroll = origin + starty - y;
                        }
                        if (moving != 0) {
                            move(windows[moving - 1], windowx + (x - startx) * 100 / deskzoom,
                                 windowy + (y - starty) * 100 / deskzoom);
                        }
                        if (x - startx > 8 || startx - x > 8 || y - starty > 8 || starty - y > 8) {
                            pressed = 0;
                        }
                        if (moving != 0 && dock(windows[moving - 1], moving - 1, width, deskzoom, deskpan, windowx, windowy)) {
                            moving = 0;
                            pressed = 0;
                            focus = 0;
                            for (slot := 0; slot < len windows; slot++) {
                                if (windows[slot] != nil && !windows[slot].hidden) {
                                    focus = slot + 1;
                                    break;
                                }
                            }
                            action = Focus;
                        }
                        changed = 1;
                    } else {
                        zooming = 0;
                        if (moving != 0) {
                            move(windows[moving - 1], windowx + (x - startx) * 100 / deskzoom,
                                 windowy + (y - starty) * 100 / deskzoom);
                        }
                        if (moving == 0 && (x - startx > 8 || startx - x > 8 ||
                                            y - starty > 8 || starty - y > 8)) {
                            pressed = 0;
                        }
                        if (pressed < 0) {
                            (left, top, wide, high) := project(-pressed - 1, width, windows[-pressed - 1], deskzoom, deskpan);
                            top -= scroll;
                            if (x < left + wide - 30 * deskzoom / 100 || x >= left + wide || y < top || y >= top + 24 * deskzoom / 100) {
                                pressed = 0;
                            }
                        }
                        if (thumbdrag >= 0 && pressed > 0) {
                            focus = pressed;
                            (scroll, deskzoom, deskpan) = restore(windows[focus - 1], focus - 1, width, height, scroll, deskzoom, deskpan);
                            if (reveal == 2147483647) {
                                raise "fail: desktop reveal epoch exhausted";
                            }
                            reveal++;
                            action = Focus;
                        }
                        if (moving != 0 && dock(windows[moving - 1], moving - 1, width, deskzoom, deskpan, windowx, windowy)) {
                            moving = 0;
                            pressed = 0;
                            focus = 1;
                            for (slot := 0; slot < len windows; slot++) {
                                if (windows[slot] != nil && !windows[slot].hidden) {
                                    focus = slot + 1;
                                    break;
                                }
                            }
                            action = Focus;
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
                        thumbdrag = -1;
                        changed = 1;
                    }
                }
            }
        }
        if (status != Ok && (request.operation == Pointer || request.operation == Scroll)) {
            dragging = 0;
            moving = 0;
            thumbdrag = -1;
            pressed = 0;
        }
        admittedfocus := livefocus(windows, focus);
        if (admittedfocus != focus) {
            focus = admittedfocus;
            changed = 1;
        }
        maximum := 72 + 9 * 288 - height;
        for (slot := 0; slot < len windows; slot++) {
            if (windows[slot] != nil) {
                (unused, bottom, unusedwide, high) := project(slot, width, windows[slot], deskzoom, deskpan);
                if (bottom + high - height + 5 > maximum) {
                    maximum = bottom + high - height + 5;
                }
            }
        }
        if (deskpan < -4096) {
            deskpan = -4096;
        }
        if (deskpan > 4096) {
            deskpan = 4096;
        }
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
                                deskzoom, deskpan, reveal, scene(windows, width, height, scroll, focus, drawer, moving, zooming, thumbnails));
        if (stop) {
            return;
        }
    }
}


# Resolve focus to an existing owner descriptor. Prefer the current visible pane,
# then another visible pane; if all are hidden, retain an existing hidden owner.
# Return0 only for an empty registry. Borrow descriptors; never restore/move/edit
# a document while correcting focus after Remove, docking or Help toggling.
livefocus(windows: array of ref Window, preferred: int): int
{
    if (preferred >= 1 && preferred <= len windows && windows[preferred - 1] != nil &&
        !windows[preferred - 1].hidden) {
        return preferred;
    }
    for (index := 0; index < len windows; index++) {
        if (windows[index] != nil && !windows[index].hidden) {
            return index + 1;
        }
    }
    if (preferred >= 1 && preferred <= len windows && windows[preferred - 1] != nil) {
        return preferred;
    }
    for (index = 0; index < len windows; index++) {
        if (windows[index] != nil) {
            return index + 1;
        }
    }
    return 0;
}


# Start one isolated geometry/gesture process. No renderer, file or document is
# acquired; only the caller owns the returned terminal request channel.
start(): chan of Request
{
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}
