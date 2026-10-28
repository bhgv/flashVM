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


# Present one selected entry and bounded path/status, never the underlying array.
# Return only copied text; renderer bounds do not truncate the native action path.
summary(sys: Sys, state: ref State): string
{
    if (state == nil) {
        return "browser unavailable";
    }
    text := "";
    if (len state.entries > 0 && state.selected >= 0 && state.selected < len state.entries) {
        entry := state.entries[state.selected];
        kind := "file ";
        if ((entry.mode & Sys->DMDIR) != 0) {
            kind = "dir ";
        }
        text += sys->sprint("page %d [%d/%d] ", state.page, state.selected+1, len state.entries);
        text += kind+preview(sys, entry.name, 400);
    } else {
        text += "empty";
    }
    if (state.ended) {
        text += " [end]";
    }
    if (state.message != "") {
        text += " | "+preview(sys, state.message, 160);
    }
    text += " | "+preview(sys, state.path, 160);
    return text;
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
