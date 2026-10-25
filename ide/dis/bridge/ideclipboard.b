implement Ideclipboard;
include "uibridge.m";
include "ideclipboard.m";


# Allocate an idle transfer owned by the caller's single Dis controller.
# No arguments; returns a new state with no native capabilities or source ownership.
create(): ref State
{
    return ref State(None, 0, 0, 0, 0, -1, 0, nil);
}


# Release the private staging root and retire all transaction identity/range values.
# state is exclusive; nil is harmless. Returns nothing and does not mutate any document.
reset(state: ref State)
{
    if (state != nil) {
        state.action = None;
        state.ticket = 0;
        state.revision = 0;
        state.first = 0;
        state.last = 0;
        state.total = -1;
        state.used = 0;
        state.bytes = nil; # [GC:] No other process may retain this mutable staging array.
    }
}


# Capture a transaction only after validating action, positive wire identity and ordered source bounds.
# Values are copied; state is exclusive. Returns one on admission, zero with no pending action.
begin(state: ref State, action, ticket, revision, first, last: int): int
{
    reset(state);
    if (state == nil || action < Copy || action > Paste || ticket < 1 || ticket > 2147483647
        || revision < 0 || revision > 2147483647 || first < 0 || last < first || last > 2147483647) {
        return 0;
    }
    state.action = action;
    state.ticket = ticket;
    state.revision = revision;
    state.first = first;
    state.last = last;
    return 1;
}


# Cancel one malformed/stale transfer without exposing its accumulated prefix.
# state is exclusive, status is a copied service error. Returns an empty, not-ready result.
failed(state: ref State, status: int): (int, int, string)
{
    reset(state);
    return (status, 0, "");
}


# Stage ordered UTF-8 bytes and publish only the complete counted Unicode value.
# Parameters and return statuses follow ideclipboard.m; no channel/native/document mutation occurs.
chunk(codec: Uibridge, state: ref State, event: ref Uibridge->Message): (int, int, string)
{
    if (codec == nil || state == nil || event == nil || state.action != Paste) {
        return failed(state, Conflict);
    }
    metadata := array of byte event.text;
    (known, ticket) := codec->parseword(metadata, 0);
    if (!known || len event.text < 8 || ticket != state.ticket || event.flags != state.revision) {
        return failed(state, Conflict);
    }
    if (event.second < 0 || event.first < 0 || event.first != state.used) {
        return failed(state, RangeError);
    }
    if (event.second > MaxBytes) {
        return failed(state, TooLarge);
    }
    body := event.text[8:];
    for (index := 0; index < len body; index++) {
        character := body[index];
        if (character <= 0 || character > 16r10ffff || (character >= 16rd800 && character <= 16rdfff)) {
            return failed(state, Invalid); # Clipboard C-string APIs cannot preserve embedded NUL.
        }
    }
    bytes := array of byte body;
    if (len bytes > PayloadBytes || len bytes > event.second - state.used
        || (len bytes == 0 && event.second != 0)) {
        return failed(state, RangeError);
    }
    if (state.total < 0) {
        state.total = event.second;
        state.bytes = array[state.total] of byte; # [GC:] Allocate only after bounded first-fragment admission.
    } else if (state.total != event.second) {
        return failed(state, RangeError);
    }
    for (offset := 0; offset < len bytes; offset++) {
        state.bytes[state.used + offset] = bytes[offset];
    }
    state.used += len bytes;
    if (state.used != state.total) {
        return (Ok, 0, ""); # The source owner never sees a partially assembled clipboard prefix.
    }
    text := string state.bytes; # Decode one complete immutable value before releasing the private buffer.
    reset(state);
    return (Ok, 1, text);
}
