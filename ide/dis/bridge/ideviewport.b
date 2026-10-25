implement Ideviewport;
include "ideviewport.m";


# Create a private viewport at the start of its future document; returns owned state.
create(): ref State
{
    return ref State(Rows, 0);
}


# Locate the logical LF line start for a bounded Unicode position; no mutation or I/O.
linebegin(text: string, position: int): int
{
    while (position > 0 && text[position - 1] != '\n') {
        position--;
    }
    return position;
}


# Advance one logical line; text/position are values, result never exceeds len text.
nextline(text: string, position: int): int
{
    while (position < len text && text[position] != '\n') {
        position++;
    }
    if (position < len text) {
        position++;
    }
    return position;
}


# Move exclusive state through acknowledged LF lines; clamp invalid offsets/delta.
# No document edit, channel wait or native geometry operation occurs.
scroll(state: ref State, text: string, delta: int)
{
    if (state == nil) {
        return;
    }
    if (state.first < 0 || state.first > len text) {
        state.first = 0;
    }
    if (delta < -128) {
        delta = -128;
    }
    if (delta > 128) {
        delta = 128;
    }
    state.first = linebegin(text, state.first);
    while (delta < 0 && state.first > 0) {
        state.first = linebegin(text, state.first - 1);
        delta++;
    }
    while (delta > 0) {
        next := nextline(text, state.first);
        # Do not scroll past the last real logical line into an invented empty one.
        if (next == state.first || (next == len text && next > 0 && text[next - 1] != '\n')) {
            break;
        }
        state.first = next;
        delta--;
    }
}


# Measure the next bounded display interval; first is an admitted scalar offset.
# Returns an exclusive end after at most Rows LF lines and Bytes UTF-8 bytes.
lastoffset(text: string, first, rows: int): int
{
    last := first;
    bytes := 0;
    lines := 0;
    while (last < len text && lines < rows) {
        rune := text[last];
        width := 1;
        if (rune >= 16r10000) {
            width = 4;
        } else if (rune >= 16r800) {
            width = 3;
        } else if (rune >= 16r80) {
            width = 2;
        }
        if (rune == '\n' && lines + 1 >= rows) {
            break;
        }
        if (bytes + width > Bytes) {
            break;
        }
        bytes += width;
        last++;
        if (rune == '\n') {
            lines++;
        }
    }
    return last;
}


# Select a bounded presentation interval, optionally revealing the acknowledged caret.
# Returns source scalar boundaries plus logical top line. No private text is copied to native pointers.
window(state: ref State, text: string, caret: int, reveal: int): (int, int, int)
{
    if (state == nil) {
        return (0, 0, 0);
    }
    if (state.first < 0 || state.first > len text) {
        state.first = 0;
    }
    if (caret < 0) {
        caret = 0;
    }
    if (caret > len text) {
        caret = len text;
    }
    rows := state.rows;
    if (rows < 1 || rows > 128) {
        rows = Rows;
    }
    last := lastoffset(text, state.first, rows);
    if (reveal && (caret < state.first || caret > last)) {
        start := linebegin(text, caret);
        state.first = start;
        # Keep context above a destination on another line without losing its caret to the byte bound.
        for (i := 1; i < rows && start > 0; i++) {
            previous := linebegin(text, start - 1);
            end := lastoffset(text, previous, rows);
            if (caret > end) {
                break;
            }
            start = previous;
            state.first = start;
        }
        last = lastoffset(text, state.first, rows);
        if (caret > last) {
            # A single long line exceeds the byte window: follow a suffix containing its caret.
            state.first = caret - 256;
            start = linebegin(text, caret);
            if (state.first < start) {
                state.first = start;
            }
            last = lastoffset(text, state.first, rows);
        }
    }
    line := 0;
    for (i := 0; i < state.first; i++) {
        if (text[i] == '\n') {
            line++;
        }
    }
    return (state.first, last, line);
}
