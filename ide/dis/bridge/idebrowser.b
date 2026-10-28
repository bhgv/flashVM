implement Idebrowser;
include "sys.m";
include "idefiles.m";
include "idebrowser.m";


# Borrow modules and allocate an exclusive controller state plus a directory owner.
# No native pointer escapes; nil dependencies fail without starting a process.
create(files: Idefiles, sys: Sys): ref State
{
    if (files == nil || sys == nil) {
        return nil;
    }
    endpoint := files->startdirs(sys);
    if (endpoint == nil) {
        return nil;
    }
    return ref State(endpoint, "", nil, big 0, 0, 0, 0, files->Ok, "", 1);
}


# Send one immutable request and consume its private correlated response.
# The last correlation identity is reserved for Shutdown so exhaustion can retire FDs.
exchange(files: Idefiles, state: ref State, operation: int, cursor: big,
         path: string): Idefiles->DirectoryReply
{
    if (state.endpoint == nil || (state.sequence == MaxRequest && operation != files->Shutdown)) {
        return Idefiles->DirectoryReply(0, files->Invalid, cursor, Sys->zerodir, nil,
                                       "browser endpoint or request identities exhausted");
    }
    identity := state.sequence;
    if (state.sequence < MaxRequest) {
        state.sequence++;
    }
    answers := chan of Idefiles->DirectoryReply;
    state.endpoint <-= Idefiles->DirectoryRequest(identity, operation, cursor, path, answers);
    response := <-answers;
    if (response.id != identity) {
        raise "fail: directory browser response correlation";
    }
    return response;
}


# Clear owned entry references only after retiring the current cursor.
# state/files are borrowed; already-clear state succeeds without native access.
clear(files: Idefiles, state: ref State): int
{
    if (state == nil || files == nil) {
        return Idefiles->Invalid;
    }
    if (state.cursor != big 0) {
        closed := exchange(files, state, files->CloseDir, state.cursor, "");
        if (closed.status != files->Ok && closed.status != files->Stale) {
            state.status = closed.status;
            state.message = closed.message;
            return state.status;
        }
    }
    state.cursor = big 0;
    state.entries = nil; # [GC:] Retire the controller's batch owner; no renderer retains it.
    state.selected = 0;
    state.page = 0;
    state.ended = 0;
    state.status = files->Ok;
    state.message = "";
    return state.status;
}


# Stage a new directory and first batch, preserving the old cursor on failure.
# path is immutable until replies are drained; publication follows all native admission.
open(files: Idefiles, state: ref State, path: string): int
{
    if (state == nil || files == nil) {
        return Idefiles->Invalid;
    }
    # Reserve admission, first read and both possible close acknowledgements.
    # Exhaustion must not strand a candidate that only Shutdown can clean up.
    if (state.sequence > MaxRequest-4) {
        state.status = files->Invalid;
        state.message = "insufficient browser request identities for transactional open";
        return state.status;
    }
    opened := exchange(files, state, files->OpenDir, big 0, path);
    if (opened.status != files->Ok) {
        state.status = opened.status;
        state.message = opened.message;
        return state.status;
    }
    first := exchange(files, state, files->NextDir, opened.cursor, "");
    if (first.status != files->Ok && first.status != files->End) {
        closed := exchange(files, state, files->CloseDir, opened.cursor, "");
        state.status = first.status;
        state.message = first.message;
        return state.status;
    }
    if (clear(files, state) != files->Ok) {
        closed := exchange(files, state, files->CloseDir, opened.cursor, "");
        return state.status;
    }
    state.path = path;
    state.cursor = opened.cursor;
    state.entries = first.entries;
    state.selected = 0;
    state.page = 1;
    state.ended = first.status == files->End;
    state.status = files->Ok;
    state.message = "";
    return state.status;
}


# Advance once; EOF keeps the last complete page, while a failed native batch
# retires the unusable cursor and releases its entry array before returning.
advance(files: Idefiles, state: ref State): int
{
    if (state == nil || files == nil || state.cursor == big 0) {
        return Idefiles->Invalid;
    }
    if (state.ended) {
        return files->End;
    }
    response := exchange(files, state, files->NextDir, state.cursor, "");
    state.status = response.status;
    state.message = response.message;
    if (response.status == files->End) {
        state.ended = 1;
    } else if (response.status == files->Ok) {
        state.entries = response.entries;
        state.selected = 0;
        state.page++;
    } else {
        state.cursor = big 0;
        state.entries = nil;
        state.selected = 0;
    }
    return state.status;
}


# Clamp a single selection step inside the private current batch, with no I/O.
# Invalid deltas/state return Invalid without changing any selection.
move(state: ref State, delta: int): int
{
    if (state == nil || (delta != -1 && delta != 1) || len state.entries == 0 ||
        state.selected < 0 || state.selected >= len state.entries) {
        return Idefiles->Invalid;
    }
    if (delta < 0 && state.selected > 0 || delta > 0 && state.selected < len state.entries-1) {
        state.selected += delta;
    }
    state.status = Idefiles->Ok;
    state.message = "";
    return state.status;
}


# Resolve the selected native name using an ordinary slash accepted by the OS ports.
# Directories navigate transactionally; file paths are values for the caller's load policy.
choose(files: Idefiles, state: ref State): (int, string)
{
    if (state == nil || files == nil || state.selected < 0 || state.selected >= len state.entries) {
        return (0, "");
    }
    selected := state.entries[state.selected];
    target := state.path;
    if (len target > 0 && target[len target-1] != '/') {
        target += "/";
    }
    target += selected.name;
    if ((selected.mode & Sys->DMDIR) != 0) {
        if (open(files, state, target) != files->Ok) {
            return (0, "");
        }
        return (PickDirectory, "");
    }
    return (PickFile, target);
}


# Copy a bounded visible name without terminal/control effects or split UTF-8.
# maxbytes is a fixed positive preview budget; the original native name is untouched.
preview(sys: Sys, text: string, maxbytes: int): string
{
    result := "";
    used := 0;
    for (index := 0; index < len text; index++) {
        rune := text[index];
        fragment := text[index:index+1];
        if (rune < 32 || (rune >= 127 && rune < 160)) {
            fragment = sys->sprint("\\u%4.4x", rune);
        }
        size := len array of byte fragment;
        if (used+size > maxbytes) {
            return result+"...";
        }
        result += fragment;
        used += size;
    }
    return result;
}


# Build a bounded listing of eight rows around the selected entry, plus toolbar.
# The 1024-byte wire budget is independent of complete native names/action paths.
summary(sys: Sys, state: ref State): string
{
    if (state == nil) {
        return "browser unavailable";
    }
    text := sys->sprint("page %d [%d/%d] ", state.page, state.selected+1, len state.entries);
    text += preview(sys, state.path, 100)+"\n";
    text += "[Up] [Next] [Refresh] [Path] [Close] [Open]\n";
    start := state.selected/8*8;
    for (index := start; index < len state.entries && index < start+8; index++) {
        entry := state.entries[index];
        kind := "FILE  ";
        if ((entry.mode & Sys->DMDIR) != 0) {
            kind = "DIR   ";
        }
        text += kind+preview(sys, entry.name, 70)+"\n";
    }
    if (len state.entries == 0) {
        text += "(empty directory)\n";
    }
    if (state.ended) {
        text += "[end] ";
    }
    text += preview(sys, state.message, 100);
    return text;
}


# Resolve copied scalar geometry against the exact escaped listing, never native names.
# sys/state are borrowed. Toolbar separators and blank/status rows have no action.
hit(sys: Sys, state: ref State, position: int): int
{
    text := summary(sys, state);
    if (state == nil || position < 0 || position >= len text) {
        return -1;
    }
    line := 0;
    start := 0;
    for (index := 0; index < position; index++) {
        if (text[index] == '\n') {
            line++;
            start = index+1;
        }
    }
    if (line == 1) {
        column := position-start;
        if (column < 4) {
            return -2;
        }
        if (column >= 5 && column < 11) {
            return -3;
        }
        if (column >= 12 && column < 21) {
            return -4;
        }
        if (column >= 22 && column < 28) {
            return -5;
        }
        if (column >= 29 && column < 36) {
            return -6;
        }
        if (column >= 37 && column < 43) {
            return -7;
        }
        return -1;
    }
    candidate := state.selected/8*8+line-2;
    if (line >= 2 && line < 10 && candidate < len state.entries) {
        return candidate;
    }
    return -1;
}


# Identify the selected row in the copied presentation for generic range highlighting.
# Bounds are Unicode scalar positions, not bytes; no heap pointer crosses the bridge.
selection(sys: Sys, state: ref State): (int, int)
{
    if (state == nil || len state.entries == 0) {
        return (0, 0);
    }
    text := summary(sys, state);
    wanted := 2+state.selected%8;
    line := 0;
    start := 0;
    for (index := 0; index < len text; index++) {
        if (text[index] == '\n') {
            if (line == wanted) {
                return (start, index);
            }
            line++;
            start = index+1;
        }
    }
    return (0, 0);
}


# Compute a lexical parent path; native filesystem admission remains in Idefiles.
# Relative paths without separators return '.', roots remain roots. May block on open.
parent(files: Idefiles, state: ref State): int
{
    if (state == nil) {
        return Idefiles->Invalid;
    }
    path := state.path;
    if (path == "." || path == "..") {
        return open(files, state, path+"/..");
    }
    end := len path;
    while (end > 1 && (path[end-1] == '/' || path[end-1] == '\\')) {
        end--;
    }
    while (end > 0 && path[end-1] != '/' && path[end-1] != '\\') {
        end--;
    }
    if (end == 0) {
        return open(files, state, ".");
    }
    return open(files, state, path[:end]);
}


# Acknowledge terminal directory shutdown before releasing the endpoint.
# Repeated calls succeed without sending to a dead service; all cursor state is retired.
shutdown(files: Idefiles, state: ref State): int
{
    if (state == nil || files == nil) {
        return Idefiles->Invalid;
    }
    if (state.endpoint == nil) {
        return files->Ok;
    }
    response := exchange(files, state, files->Shutdown, big 0, "");
    if (response.status == files->Ok) {
        state.endpoint = nil;
        state.cursor = big 0;
        state.entries = nil;
        state.selected = 0;
        state.page = 0;
    }
    state.status = response.status;
    state.message = response.message;
    return state.status;
}
