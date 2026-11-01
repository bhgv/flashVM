implement Idedesktop;
include "idedesktop.m";
include "idetabview.m";
include "idedocking.m";
include "idepick.m";
include "idethumbnails.m";
include "iderails.m";
include "idetools.m";
include "idetoolpalette.m";

palettes: Idetoolpalette; # Pure owner-anchored popup geometry.
rails: Iderails; # Pure independent rail geometry.
tools: Idetools; # Labels and command palette rows, never document state.
thumbview: Idethumbnails; # Pure shelf geometry and order mapping.
picking: Idepick; # Pure reversed-scene admission, shared with actual paint order.
tabs: Idetabview; # Pure geometry module; the layout process owns selection.
docking: Idedocking; # Pure contact policy; the layout process owns hidden state.


# Copied presentation state; document authority stays with its separate pane owner.
# [GC:] Parent references form a depth-one forest inside this owner. Replies carry
# copied scalar geometry only; no presentation reference crosses a VM heap.
Window: adt
{
    container: ref Window; # Private presentation parent; nil means SDI.
    root: int; # Container identity1..8, zero for document windows.
    family: int; # Container domain1 Limbo/2 graphics/3 ActionScript.
    rank: int; # Child tab ordinal; on containers, selected child identity.
    count: int; # Number of children for container geometry.
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
    if (window.container != nil) {
        parent := window.container;
        (x, y, wide, high) := position(parent.root - 1, width, parent);
        return tabs->body(x, y, wide, high);
    }
    (x, y, wide, high) := geometry(index, width);
    if (window.root != 0) {
        x = 96;
        y = 72 + (window.root - 1) * 320;
        wide = width - 216;
        if (wide < 96) {
            wide = 96;
        }
        high = 400;
    }
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
project(index, width: int, window: ref Window, zoom, pan, scroll: int): (int, int, int, int)
{
    (x, y, wide, high) := position(index, width, window);
    if (viewportfixed(window)) {
        return (x, y + scroll, wide, high);
    }
    return (84 + (x - 84) * zoom / 100 + pan, 48 + (y - 48) * zoom / 100,
            wide * zoom / 100, high * zoom / 100);
}


# Toggle max/normal without overwriting the saved ordinary position or size.
# Inputs are bounded copied canvas values; no document ownership is acquired.
maximize(window: ref Window, width, height, scroll, zoom: int)
{
    if (window.container != nil) {
        maximize(window.container, width, height, scroll, zoom);
        return;
    }
    window.maximized = !window.maximized;
    window.maxwide = width - tools->RightWidth - 92;
    window.maxhigh = height - 60;
    window.maxtop = 48;
}


# Apply a bounded absolute drag proposal in table pixels. A maximized window
# preserves normal offsets; the caller borrows its presentation-only record.
move(window: ref Window, dx, dy: int)
{
    if (window.maximized || window.container != nil) {
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
    if (window.maximized) {
        return (scroll, zoom, pan);
    }
    if (window.container != nil) {
        parent := window.container;
        return restore(parent, parent.root - 1, width, height, scroll, zoom, pan);
    }
    (originx, originy, wide, high) := position(index, width, window);
    # Keep one spare pixel for fractional transforms and round exposed bounds
    # outward; integer truncation must not hide a sliver beneath a rail.
    fit := (width - tools->RightWidth - 91) * 100 / wide;
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
    if (right > width - tools->RightWidth - 5) {
        pan -= right - (width - tools->RightWidth - 5);
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


# Hide a dragged SDI or MDI card when its contact enters the shelf. Preserve
# pre-drag offsets for reveal; only the layout owner mutates this presentation.
dock(window: ref Window, x, y, dx, dy: int): int
{
    if (!docking->inside(x, y)) {
        return 0;
    }
    if (window.container != nil) {
        window = window.container;
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
texttarget(windows: array of ref Window, width, height, scroll, focus, drawer, x, y, zoom, pan, rightscroll: int): int
{
    if (x < 80 || x >= width - tools->RightWidth || y < 48 || y >= height) {
        return 0;
    }
    if (tools->palette(drawer)) {
        labels := tools->rows(drawer);
        owner_y := tools->ButtonTop + (tools->index(drawer) - 1) * tools->ButtonStep - rightscroll;
        (px, py, pw, ph) := palettes->bounds(owner_y, len labels, drawer == tools->Files, width, height);
        if (palettes->contains(x, y, px, py, pw, ph)) {
            return 0;
        }
    }
    left := width - 424;
    if (left < 84) {
        left = 84;
    }
    if (drawer != 0 && !tools->palette(drawer) && x >= left && x < width - tools->RightWidth - 8 && y >= 72 && y < 412) {
        return 0;
    }
    target := 0;
    for (order := 0; order < len windows; order++) {
        index := stackslot(order, focus);
        if (windows[index] == nil || windows[index].hidden ||
            (windows[index].container != nil && (windows[index].container.rank != index + 1 || windows[index].container.hidden))) {
            continue;
        }
        (x0, y0, wide, high) := project(index, width, windows[index], zoom, pan, scroll);
        y0 -= scroll;
        if (x >= x0 && x < x0 + wide && y >= y0 && y < y0 + high) {
            target = 0;
            if (y >= y0 + 24 * controlzoom(windows[index], zoom) / 100) {
                target = index + 1;
            }
        }
    }
    return target;
}


# Build fresh scene descriptors. windows is borrowed only during this call;
# the returned array/Items are new, and all text values are immutable Dis strings.
scene(windows: array of ref Window, width, height, scroll, focus, drawer, moving, zooming: int, thumbnails: array of int, containers: array of ref Window, leftscroll, rightscroll: int, recent: array of string): array of ref Item
{
    items := array[94] of ref Item;
    used := 0;
    focused: ref Window;
    if (focus >= 1 && focus <= len windows && windows[focus - 1] != nil) {
        focused = windows[focus - 1].container;
    }
    items[used++] = ref Item(10, 0, 0, 0, 0, width, height, 2, "", -1, -1);
    for (group := 0; group < len containers; group++) {
        parent := containers[group];
        if (parent == nil || parent == focused) {
            continue;
        }
        (gx, gy, gw, gh) := position(group, width, parent);
        id := 13 + group;
        if (group == 7) {
            id = 28;
        }
        if (parent.hidden) {
            gx = 65536;
            gy = 65536;
        }
        items[used++] = ref Item(id, layer(parent), 0, gx, gy, gw, gh, 4, parent.title, -1, -1);
    }
    for (order := 0; order < len windows; order++) {
        index := stackslot(order, focus);
        window := windows[index];
        if (window == nil) {
            continue;
        }
        # The focused MDI is one paint-order unit: root, tabs, selected child.
        # Its header must not sit behind another group's taller content pane.
        if (index + 1 == focus && focused != nil) {
            (gx, gy, gw, gh) := position(focused.root - 1, width, focused);
            rootid := 12 + focused.root;
            if (focused.root == 8) {
                rootid = 28;
            }
            if (focused.hidden) {
                gx = 65536;
                gy = 65536;
            }
            items[used++] = ref Item(rootid, layer(focused), 0, gx, gy, gw, gh, 4, focused.title, -1, -1);
            for (child := 0; child < len windows; child++) {
                tab := windows[child];
                if (tab == nil || tab.hidden || tab.container != focused || focused.hidden) {
                    continue;
                }
                tabwide := tabs->width(gw, focused.count, 100);
                tabstyle := 4;
                if (child + 1 == focus) {
                    tabstyle = 1;
                }
                items[used++] = ref Item(30 + child, layer(focused), 0, gx + 8 + tab.rank * tabwide,
                    gy + 28, tabwide - 2, 26, tabstyle, tab.title, -1, -1);
            }
        }
        (x, y, wide, high) := position(index, width, window);
        if (window.hidden || (window.container != nil && (window.container.rank != index + 1 || window.container.hidden))) {
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
        parentid := layer(window);
        if (window.container != nil && !window.hidden && window.container.rank == index + 1 && !window.container.hidden) {
            parent := window.container;
            (gx, gy, unusedw, unusedh) := position(parent.root - 1, width, parent);
            x -= gx;
            y -= gy;
            parentid = 4 + parent.root;
        }
        items[used++] = ref Item(index + 1, parentid, 0, x, y, wide, high, style + window.line * 8, caption, caret, anchor);
        if (window.container != nil && window.container != focused && !window.hidden && !window.container.hidden) {
            (gx, gy, gw, gh) := position(window.container.root - 1, width, window.container);
            tabwide := tabs->width(gw, window.container.count, 100);
            tabstyle := 4;
            if (window.container.rank == index + 1 && !window.container.hidden) {
                tabstyle = 1;
            }
            items[used++] = ref Item(30 + index, layer(window), 0, gx + 8 + window.rank * tabwide,
                gy + 28, tabwide - 2, 26, tabstyle, window.title, -1, -1);
        } else if (window.container == nil && index != 1 && index != 2 && !window.hidden) {
            (closex, closey, closew, closeh) := position(index, width, window);
            items[used++] = ref Item(30 + index, layer(window), 0, closex + wide - 30, closey, 30, 24, 4, "x", -1, -1);
        }
    }
    items[used++] = ref Item(11, 2, 0, 0, 0, 80, height, 2, "", -1, -1);
    items[used++] = ref Item(12, 3, 0, 0, 0, tools->RightWidth, height, 2, "", -1, -1);
    thumbnailcount := 0;
    for (slot := 0; slot < len windows; slot++) {
        thumbnailcount += windows[slot] != nil;
    }
    thumbstep := thumbview->step(thumbnailcount, height);
    ordinal := 0;
    for (rank := 0; rank < len thumbnails; rank++) {
        index := thumbnails[rank];
        if (windows[index] != nil) {
            items[used++] = ref Item(20 + index, 2, index + 1, 6, 72 + ordinal * thumbstep - leftscroll, 68, thumbstep - 8,
                                    index + 1 == focus, windows[index].title, -1, -1);
            ordinal++;
        }
    }
    names := array[] of {"", "Open", "Save", "Build", "New", "Debug", "Help", "Quit", "Close", "Clip", "All", "Max/N", "Draw", "MDI", "SDI"};
    for (index := 1; index <= tools->Count; index++) {
        command := tools->action(index);
        style := 4;
        if (drawer == command || (command == Help && windows[2] != nil && !windows[2].hidden)) {
            style = 3;
        }
        items[used++] = ref Item(tools->identity(index), 3, 0, tools->ButtonLeft,
            tools->ButtonTop + (index - 1) * tools->ButtonStep - rightscroll,
            tools->ButtonWidth, tools->ButtonHeight, style, tools->name(index), -1, -1);
    }
    if (tools->palette(drawer)) {
        labels := tools->rows(drawer);
        owner_y := tools->ButtonTop + (tools->index(drawer) - 1) * tools->ButtonStep - rightscroll;
        (x, y, wide, high) := palettes->bounds(owner_y, len labels, drawer == tools->Files, width, height);
        items[used++] = ref Item(55, 4, 0, x, y, wide, high, 2, "", -1, -1);
        items[used++] = ref Item(71, 4, 0, x + wide - 1, owner_y + 5, 12, 16, 2, "", -1, -1);
        for (row := 0; row < len labels; row++) {
            (cx, cy) := palettes->cell(row, x, y, height);
            items[used++] = ref Item(72 + row, 4, 0, cx, cy,
                tools->ButtonWidth, tools->ButtonHeight, 4, labels[row], -1, -1);
        }
        if (drawer == tools->Files && palettes->legend(height) > 0) {
            notes := "";
            for (index := 0; index < len recent; index++) {
                label := recent[index];
                if (len label > 26) {
                    label = label[:3] + "..." + label[len label - 20:];
                }
                notes += label + "\n";
            }
            items[used++] = ref Item(88, 4, 0, x + 8, y + high - palettes->legend(height), wide - 16, palettes->legend(height) - 4, 2, notes, -1, -1);
        }
    } else if (drawer == tools->Find || drawer == tools->Replace) {
        x := width - 424;
        if (x < 84) {
            x = 84;
        }
        text := "Find in file\nLiteral, case-sensitive Unicode search. Enter finds the next match and wraps. Escape cancels.";
        if (drawer == tools->Replace) {
            text = "Replace all in file\nEnter the literal search text, then the replacement. Second Enter applies one undoable replacement. Escape cancels.";
        }
        items[used++] = ref Item(55, 4, 0, x, 72, width - tools->RightWidth - 8 - x, 340, 0, text, -1, -1);
    } else if (drawer != 0) {
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
        wide := width - tools->RightWidth - 8 - x;
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
    containers := array[MaxWindows] of ref Window; # Presentation roots only; no document/GC owner transfer.
    groupdrag := 0; # Container being moved; document drag state is independent.
    thumbnails := array[MaxWindows] of int;
    for (rank := 0; rank < len thumbnails; rank++) {
        thumbnails[rank] = rank;
    }
    recent := array[] of {"1: (empty)", "2: (empty)", "3: (empty)", "4: (empty)"}; # Display labels only.
    leftscroll := 0; # Independent physical left-shelf offset.
    rightscroll := 0; # Independent physical toolbar offset.
    raildrag := 0; # Latched2/3 rail for pan gestures, zero for table.
    railorigin := 0; # Offset captured at the beginning of a rail drag.
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
        present := array[MaxWindows] of { * => 0 };
        live := 0;
        for (presenceindex := 0; presenceindex < len windows; presenceindex++) {
            present[presenceindex] = windows[presenceindex] != nil;
            live += present[presenceindex];
        }
        thumbstep := thumbview->step(live, height);
        status := Invalid;
        action := 0;
        changed := 0;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Shutdown) {
                status = Ok;
                stop = 1;
            } else if (request.operation == RecentLabels && len request.body <= 160) {
                first := 0;
                row := 0;
                for (index := 0; index < len request.body && row < 4; index++) {
                    if (request.body[index] == '\n') {
                        recent[row++] = request.body[first:index];
                        first = index + 1;
                    }
                }
                status = Ok;
                changed = 1;
            } else if (request.operation == SetContainer && request.window >= 1 && request.window <= MaxWindows &&
                       windows[request.window - 1] != nil && request.phase >= 0 && request.phase <= MaxWindows &&
                       (request.x >= 1 && request.x <= 3)) {
                target := request.phase - 1;
                status = Ok;
                if (target >= 0 && containers[target] != nil && containers[target].family != request.x) {
                    status = Invalid;
                }
                if (status == Ok) {
                    if (target >= 0 && containers[target] == nil) {
                        name := "Limbo MDI";
                        if (request.x == 2) {
                            name = "Graphics MDI";
                        } else if (request.x == 3) {
                            name = "ActionScript MDI";
                        }
                        containers[target] = ref Window(nil, target + 1, request.x, 0, 0,
                            name, "", -1, -1, 0, 0, 0, 0, 0, 0, 100, 0, 0);
                    }
                    windows[request.window - 1].container = nil;
                    if (target >= 0) {
                        windows[request.window - 1].container = containers[target];
                    }
                    dragging = 0;
                    moving = 0;
                    groupdrag = 0;
                    pressed = 0;
                    thumbdrag = -1;
                    changed = 1;
                }
            } else if (request.operation == Cancel) {
                zooming = 0;
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
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
                windows[request.window - 1] = ref Window(nil, 0, 0, 0, 0, request.title, request.body, request.caret, request.anchor, 0, 0, 0, 0, request.line, hidden, zoom, dx, dy);
                if (old != nil) {
                    current := windows[request.window - 1];
                    current.container = old.container;
                    current.rank = old.rank;
                    current.maximized = old.maximized;
                    current.maxwide = old.maxwide;
                    current.maxhigh = old.maxhigh;
                    current.maxtop = old.maxtop;
                }
                status = Ok;
            } else if (request.operation == Remove && request.window >= 1 && request.window <= MaxWindows &&
                       request.window != 2 && request.window != 3 && windows[request.window - 1] != nil) {
                windows[request.window - 1] = nil;
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
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
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
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
                for (group := 0; group < len containers; group++) {
                    if (containers[group] != nil && containers[group].maximized) {
                        containers[group].maximized = 0;
                        maximize(containers[group], width, height, scroll, deskzoom);
                    }
                }
                changed = 1;
                status = Ok;
            } else if (request.operation == TextHit) {
                status = Conflict;
                if (request.ticket == ticket && request.window >= 1 && request.window <= MaxWindows &&
                    texttarget(windows, width, height, scroll, focus, drawer, request.x, request.y, deskzoom, deskpan, rightscroll) == request.window) {
                    status = Ok;
                }
            } else if (request.operation == Activate && request.window >= 1 && request.window <= MaxWindows && windows[request.window - 1] != nil) {
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
                thumbdrag = -1;
                pressed = 0;
                focus = request.window;
                changed = 1;
                status = Ok;
            } else if (request.operation == Select && request.window >= 1 && request.window <= MaxWindows && windows[request.window - 1] != nil) {
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
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
            } else if (request.operation == Tool && request.window >= 0 && (request.window <= MaxNorm || (tools->palette(request.window) || request.window == tools->Find || request.window == tools->Replace))) {
                raildrag = 0;
                dragging = 0;
                moving = 0;
                groupdrag = 0;
                thumbdrag = -1;
                pressed = 0;
                drawer = request.window;
                changed = 1;
                status = Ok;
            } else if (request.operation == Pointer || request.operation == Scroll || request.operation == RailScroll) {
                if (request.ticket != ticket) {
                    status = Conflict;
                    dragging = 0;
                    moving = 0;
                    groupdrag = 0;
                    thumbdrag = -1;
                    pressed = 0;
                } else if (request.operation == RailScroll && (request.window == 2 || request.window == 3) && request.x >= -2048 && request.x <= 2048) {
                    raildrag = 0;
                    dragging = 0;
                    moving = 0;
                    groupdrag = 0;
                    thumbdrag = -1;
                    pressed = 0;
                    if (request.window == 2) {
                        leftscroll += request.x;
                    } else {
                        rightscroll += request.x;
                    }
                    changed = 1;
                    status = Ok;
                } else if (request.operation == Scroll && request.x >= -2048 && request.x <= 2048) {
                    dragging = 0;
                    moving = 0;
                    groupdrag = 0;
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
                    popupx := 0;
                    popupy := 0;
                    popupwide := 0;
                    popuphigh := 0;
                    popupcount := 0;
                    if (tools->palette(drawer)) {
                        popupcount = len tools->rows(drawer);
                        owner_y := tools->ButtonTop + (tools->index(drawer) - 1) * tools->ButtonStep - rightscroll;
                        (popupx, popupy, popupwide, popuphigh) = palettes->bounds(owner_y, popupcount,
                            drawer == tools->Files, width, height);
                    }
                    inpopup := tools->palette(drawer) && palettes->contains(x, y, popupx, popupy, popupwide, popuphigh);
                    if (phase == 4 || phase == 5 || phase == 6) {
                        moving = 0;
                        groupdrag = 0;
                        thumbdrag = -1;
                        dragging = 0;
                        pressed = 0;
                        target := 0;
                        obscured := inpopup || (drawer != 0 && !tools->palette(drawer) && x >= drawerleft && x < width - tools->RightWidth - 8 && y >= 72 && y < 412);
                        for (order := 0; order < len windows; order++) {
                            index := stackslot(order, focus);
                            if (obscured || windows[index] == nil || windows[index].hidden ||
            (windows[index].container != nil && (windows[index].container.rank != index + 1 || windows[index].container.hidden))) {
                                continue;
                            }
                            (left, top, wide, high) := project(index, width, windows[index], deskzoom, deskpan, scroll);
                            top -= scroll;
                            if (x >= left && x < left + wide && y >= top && y < top + high) {
                                target = 0;
                                if (y < top + 24 * controlzoom(windows[index], deskzoom) / 100) {
                                    target = index + 1;
                                }
                            }
                        }
                        if (!obscured && target == 0) {
                            for (group := 0; group < len containers; group++) {
                                parent := containers[group];
                                if (parent == nil || parent.hidden) {
                                    continue;
                                }
                                (gx, gy, gw, gh) := project(group, width, parent, deskzoom, deskpan, scroll);
                                gy -= scroll;
                                if (x >= gx && x < gx + gw && y >= gy &&
                                    y < gy + 28 * controlzoom(parent, deskzoom) / 100) {
                                    target = parent.rank;
                                }
                            }
                        }
                        if (phase != 6 && !obscured && target == 0 && x >= 80 && x < width - tools->RightWidth && y >= 48) {
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
                            maximize(windows[target - 1], width, height, scroll, deskzoom);
                            focus = target;
                            action = Focus;
                            changed = 1;
                        } else if (target != 0 && windows[target - 1].container == nil && !viewportfixed(windows[target - 1])) {
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
                        groupdrag = 0;
                        thumbdrag = -1;
                        pressed = 0;
                        thumbdrag = -1;
                        picked := picking->topmost(scene(windows, width, height, scroll, focus, drawer,
                            moving, zooming, thumbnails, containers, leftscroll, rightscroll, recent), x, y, width, deskzoom, deskpan, scroll);
                        startx = x;
                        starty = y;
                        origin = scroll;
                        raildrag = 0;
                        rail := rails->region(x, y, width);
                        railpan := fingers == 3 ||
                            (rail == 2 && (x < 6 || x >= 74 || y < 72 || (y - 72 + leftscroll) % thumbstep >= thumbstep - 8)) ||
                            (rail == 3 && (x < width - tools->RightWidth + tools->ButtonLeft || x >= width - 6 ||
                                y < tools->ButtonTop || (y - tools->ButtonTop + rightscroll) % tools->ButtonStep >= tools->ButtonHeight));
                        if (rail != 0 && railpan && !inpopup) {
                            raildrag = rail;
                            railorigin = leftscroll;
                            if (rail == 3) {
                                railorigin = rightscroll;
                            }
                        } else if (fingers == 3 && rail == 0) {
                            drawer = 0;
                            dragging = 1;
                        } else if (inpopup && fingers == 1) {
                            row := palettes->hit(x, y, popupx, popupy, popupcount, height);
                            if (row >= 0) {
                                action = tools->command(drawer, row);
                                drawer = 0;
                            }
                        } else if (drawer == ClipboardTool && y >= 320 && y < 356) {
                            wide := width - tools->RightWidth - 8 - drawerleft;
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
                        } else if (drawer == CloseEditor && x >= drawerleft + 8 && x < width - tools->RightWidth - 16 && y >= 364 && y < 400) {
                            action = CancelClose;
                            drawer = 0;
                        } else if (drawer != 0 && !tools->palette(drawer) && x >= drawerleft + 8 && x < width - tools->RightWidth - 16 && y >= 320 && y < 356) {
                            action = drawer;
                            drawer = 0;
                        } else if (drawer != 0 && !tools->palette(drawer) && x >= drawerleft && x < width - tools->RightWidth - 8 && y >= 72 && y < 412) {
                            # The scroll owns this area; never focus or drag the covered window.
                        } else if (x >= width - tools->RightWidth && y >= 56) {
                            ordinal := (y - tools->ButtonTop + rightscroll) / tools->ButtonStep + 1;
                            command := tools->action(ordinal);
                            if (tools->palette(command)) {
                                if (drawer == command) {
                                    drawer = 0;
                                } else {
                                    drawer = command;
                                }
                            } else if (command == Help && windows[2] != nil) {
                                windows[2].hidden = !windows[2].hidden;
                                drawer = 0;
                                focus = 3;
                                if (windows[2].hidden) {
                                    focus = 1;
                                }
                                action = Focus;
                            } else {
                                drawer = 0;
                                action = command;
                            }
                        } else if (x < 80 && y >= 72) {
                            rank := (y - 72 + leftscroll) / thumbstep;
                            slot := -1;
                            rank = thumbview->slot(thumbnails, present, rank);
                            if (rank >= 0) {
                                slot = thumbnails[rank];
                            }
                            if (slot >= 0 && windows[slot] != nil) {
                                thumbdrag = rank;
                                pressed = slot + 1;
                            }
                        } else if (x >= 80 && x < width - tools->RightWidth && y >= 48) {
                            tabhit := 0;
                            if (fingers == 1) {
                                for (child := 0; child < len windows; child++) {
                                    window := windows[child];
                                    if (window == nil || window.hidden || window.container == nil || picked != 30 + child) {
                                        continue;
                                    }
                                    parent := window.container;
                                    (gx, gy, gw, gh) := project(parent.root - 1, width, parent, deskzoom, deskpan, scroll);
                                    gy -= scroll;
                                    if (tabs->hit(x, y, gx, gy, gw, parent.count, window.rank, controlzoom(parent, deskzoom))) {
                                        focus = child + 1;
                                        parent.rank = focus;
                                        pressed = focus;
                                        tabhit = 1;
                                        action = Focus;
                                    }
                                }
                            }
                            for (order := 0; order < len windows; order++) {
                                index := stackslot(order, focus);
                                if (tabhit || fingers == 3 || (picked != index + 1 && picked != 30 + index) || windows[index] == nil || windows[index].hidden ||
            (windows[index].container != nil && (windows[index].container.rank != index + 1 || windows[index].container.hidden))) {
                                    continue;
                                }
                                (left, top, wide, high) := project(index, width, windows[index], deskzoom, deskpan, scroll);
                                top -= scroll;
                                if (x >= left && x < left + wide && y >= top && y < top + high) {
                                    pressed = index + 1;
                                    moving = 0;
                                    groupdrag = 0;
                                    thumbdrag = -1;
                                    if (y < top + 24 * controlzoom(windows[index], deskzoom) / 100 || fingers == 2) {
                                        if (fingers == 1 && x >= left + wide - 30 * controlzoom(windows[index], deskzoom) / 100 && index != 1 && index != 2 && windows[index].container == nil) {
                                            pressed = -(index + 1);
                                        } else {
                                            moving = index + 1;
                                            windowx = windows[index].dx;
                                            windowy = windows[index].dy;
                                            if (windows[index].container != nil) {
                                                windowx = windows[index].container.dx;
                                                windowy = windows[index].container.dy;
                                            }
                                        }
                                    }
                                }
                            }
                            if (pressed == 0 && drawer == 0) {
                                for (group := 0; group < MaxWindows; group++) {
                                    if (containers[group] == nil) {
                                        continue;
                                    }
                                    (gx, gy, gw, gh) := project(group, width, containers[group], deskzoom, deskpan, scroll);
                                    gy -= scroll;
                                    rootid := 13 + group;
                                    if (group == 7) {
                                        rootid = 28;
                                    }
                                    if (fingers != 3 && picked == rootid && x >= gx && x < gx + gw && y >= gy && y < gy + 28 * controlzoom(containers[group], deskzoom) / 100) {
                                        groupdrag = group + 1;
                                        windowx = containers[group].dx;
                                        windowy = containers[group].dy;
                                        focus = containers[group].rank;
                                        action = Focus;
                                    }
                                }
                            }
                            dragging = pressed == 0 && drawer == 0 && groupdrag == 0;
                            if (moving != 0) {
                                focus = moving;
                                action = Focus;
                            }
                        }
                        changed = 1;
                    } else if (phase == 2) {
                        if (groupdrag != 0 && containers[groupdrag - 1] != nil) {
                            move(containers[groupdrag - 1], windowx + (x - startx) * 100 / deskzoom,
                                 windowy + (y - starty) * 100 / deskzoom);
                        }
                        if (thumbdrag >= 0) {
                            rank := (y - 72 + leftscroll) / thumbstep;
                            if (rank < 0) {
                                rank = 0;
                            }
                            if (rank >= live) {
                                rank = live - 1;
                            }
                            rank = thumbview->slot(thumbnails, present, rank);
                            if (rank < 0) {
                                rank = thumbdrag;
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
                        if (raildrag == 2) {
                            leftscroll = railorigin + starty - y;
                        } else if (raildrag == 3) {
                            rightscroll = railorigin + starty - y;
                        }
                        if (dragging) {
                            scroll = origin + starty - y;
                        }
                        if (moving != 0) {
                            target := windows[moving - 1];
                            if (target.container != nil) {
                                target = target.container;
                            }
                            move(target, windowx + (x - startx) * 100 / deskzoom,
                                 windowy + (y - starty) * 100 / deskzoom);
                        }
                        if (x - startx > 8 || startx - x > 8 || y - starty > 8 || starty - y > 8) {
                            pressed = 0;
                        }
                        raildrag = 0;
                        docked := 0;
                        if (moving != 0) {
                            docked = dock(windows[moving - 1], x, y, windowx, windowy);
                        } else if (groupdrag != 0 && containers[groupdrag - 1] != nil) {
                            docked = dock(containers[groupdrag - 1], x, y, windowx, windowy);
                        }
                        if (docked) {
                            moving = 0;
                            groupdrag = 0;
                            pressed = 0;
                            focus = 0;
                            for (slot := 0; slot < len windows; slot++) {
                                if (windows[slot] != nil && !windows[slot].hidden &&
                                    (windows[slot].container == nil || !windows[slot].container.hidden)) {
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
                            target := windows[moving - 1];
                            if (target.container != nil) {
                                target = target.container;
                            }
                            move(target, windowx + (x - startx) * 100 / deskzoom,
                                 windowy + (y - starty) * 100 / deskzoom);
                        }
                        if (moving == 0 && (x - startx > 8 || startx - x > 8 ||
                                            y - starty > 8 || starty - y > 8)) {
                            pressed = 0;
                        }
                        if (pressed < 0) {
                            (left, top, wide, high) := project(-pressed - 1, width, windows[-pressed - 1], deskzoom, deskpan, scroll);
                            top -= scroll;
                            if (x < left + wide - 30 * controlzoom(windows[-pressed - 1], deskzoom) / 100 || x >= left + wide || y < top || y >= top + 24 * controlzoom(windows[-pressed - 1], deskzoom) / 100) {
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
                        raildrag = 0;
                        docked := 0;
                        if (moving != 0) {
                            docked = dock(windows[moving - 1], x, y, windowx, windowy);
                        } else if (groupdrag != 0 && containers[groupdrag - 1] != nil) {
                            docked = dock(containers[groupdrag - 1], x, y, windowx, windowy);
                        }
                        if (docked) {
                            moving = 0;
                            groupdrag = 0;
                            pressed = 0;
                            focus = 1;
                            for (slot := 0; slot < len windows; slot++) {
                                if (windows[slot] != nil && !windows[slot].hidden &&
                                    (windows[slot].container == nil || !windows[slot].container.hidden)) {
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
                        groupdrag = 0;
                        thumbdrag = -1;
                        changed = 1;
                    }
                }
            }
        }
        if (status != Ok && (request.operation == Pointer || request.operation == Scroll || request.operation == RailScroll)) {
            raildrag = 0;
            dragging = 0;
            moving = 0;
            groupdrag = 0;
            thumbdrag = -1;
            pressed = 0;
        }
        if (action == MaxNorm && focus >= 1 && windows[focus - 1] != nil) {
            maximize(windows[focus - 1], width, height, scroll, deskzoom);
            action = 0;
            changed = 1;
        } else if (action == ShowAll) {
            # Restore every normal rectangle before measuring any child/root.
            for (resetwindow := 0; resetwindow < len windows; resetwindow++) {
                if (windows[resetwindow] != nil) {
                    windows[resetwindow].maximized = 0;
                }
            }
            for (resetgroup := 0; resetgroup < len containers; resetgroup++) {
                if (containers[resetgroup] != nil) {
                    containers[resetgroup].maximized = 0;
                }
            }
            minimumleft := 84;
            right := 85;
            bottom := 49;
            for (slot := 0; slot < len windows; slot++) {
                if (windows[slot] == nil) {
                    continue;
                }
                windows[slot].hidden = 0;
                windows[slot].maximized = 0;
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
            for (group := 0; group < MaxWindows; group++) {
                if (containers[group] == nil) {
                    continue;
                }
                containers[group].hidden = 0;
                containers[group].maximized = 0;
                (left, top, wide, high) := position(group, width, containers[group]);
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
            deskzoom = (width - tools->RightWidth - 92) * 100 / (right - minimumleft);
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
            action = 0;
            changed = 1;
        }
        admittedfocus := livefocus(windows, focus);
        if (admittedfocus != focus) {
            focus = admittedfocus;
            changed = 1;
        }
        for (group := 0; group < MaxWindows; group++) {
            parent := containers[group];
            if (parent == nil) {
                continue;
            }
            parent.count = 0;
            selected := 0;
            for (child := 0; child < MaxWindows; child++) {
                if (windows[child] != nil && windows[child].container == parent) {
                    windows[child].rank = parent.count++;
                    if (parent.rank == child + 1) {
                        selected = child + 1;
                    }
                    if (selected == 0) {
                        selected = child + 1;
                    }
                    if (focus == child + 1) {
                        parent.rank = child + 1;
                        selected = child + 1;
                    }
                }
            }
            parent.rank = selected;
            if (parent.count == 0) {
                containers[group] = nil;
            }
        }
        maximum := 72 + 9 * 288 - height;
        for (slot := 0; slot < len windows; slot++) {
            if (windows[slot] != nil) {
                (unused, bottom, unusedwide, high) := project(slot, width, windows[slot], deskzoom, deskpan, scroll);
                if (bottom + high - height + 5 > maximum) {
                    maximum = bottom + high - height + 5;
                }
            }
        }
        for (rootindex := 0; rootindex < MaxWindows; rootindex++) {
            if (containers[rootindex] != nil) {
                (unused, bottom, unusedwide, high) := project(rootindex, width, containers[rootindex], deskzoom, deskpan, scroll);
                if (bottom + high - height + 5 > maximum) {
                    maximum = bottom + high - height + 5;
                }
            }
        }
        leftscroll = rails->clamp(leftscroll, live, thumbstep, 72, height);
        rightscroll = rails->clamp(rightscroll, tools->Count, tools->ButtonStep, tools->ButtonTop, height);
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
                                deskzoom, deskpan, reveal, scene(windows, width, height, scroll, focus, drawer, moving, zooming, thumbnails, containers, leftscroll, rightscroll, recent));
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
        !windows[preferred - 1].hidden && (windows[preferred - 1].container == nil ||
        !windows[preferred - 1].container.hidden)) {
        return preferred;
    }
    for (index := 0; index < len windows; index++) {
        if (windows[index] != nil && !windows[index].hidden &&
            (windows[index].container == nil || !windows[index].container.hidden)) {
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
    palettes = load Idetoolpalette Idetoolpalette->PATH;
    rails = load Iderails Iderails->PATH;
    tools = load Idetools Idetools->PATH;
    thumbview = load Idethumbnails Idethumbnails->PATH;
    picking = load Idepick Idepick->PATH;
    tabs = load Idetabview Idetabview->PATH;
    docking = load Idedocking Idedocking->PATH;
    if (tabs == nil || docking == nil || picking == nil || thumbview == nil || rails == nil || tools == nil || palettes == nil) {
        raise "fail: desktop control modules";
    }
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}


# A maximized SDI or selected child of a maximized MDI uses physical canvas units.
# Borrow presentation metadata only; no document ownership or channel operation.
viewportfixed(window: ref Window): int
{
    return window.maximized || (window.container != nil && window.container.maximized);
}


# Choose the existing unscaled overlay group for maximized chrome/content. Normal
# panes remain on the transformed desktop. Return a copied scene parent identity.
layer(window: ref Window): int
{
    if (viewportfixed(window)) {
        return 4;
    }
    return 1;
}


# Use native control geometry for fixed panes and the desktop scale otherwise.
# The returned percent is for hit admission only; stored ordinary zoom is untouched.
controlzoom(window: ref Window, zoom: int): int
{
    if (viewportfixed(window)) {
        return 100;
    }
    return zoom;
}
