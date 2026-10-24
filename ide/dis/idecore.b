implement Idecore;
include "idecore.m";

# [GC:] Private immutable snapshot stacks belong to the document owner. Replies
# contain text values, never mutable History references or list nodes.
History: adt
{
    undo: list of string;   # Most recent previous text first.
    redo: list of string;   # Most recent undone text first.
};

# Private state belongs to one service process; it is never exposed as a mutable UI object.
Document: adt
{
    opened: int;       # Whether text represents an open document.
    revision: int;     # Never reset by close/reopen, so stale edits cannot acquire a new session.
    saved: int;        # The last explicitly acknowledged clean revision.
    text: string;      # Character-indexed document content, bounded by MaxCharacters.
    history: ref History; # Private bounded Undo/Redo roots; cleared across sessions.
};

# Create a typed channel boundary; no native UI or host-runtime call occurs here.
start(): chan of Request
{
    commands := chan of Request;
    document := ref Document(0, 0, 0, "", ref History(nil, nil));
    spawn serve(commands, document);
    return commands;
}

# Copy observable state into a response without transferring the private mutable document.
snapshot(document: ref Document, id: int, status: int): Reply
{
    return Reply(id, status, document.opened, document.revision, document.saved, document.text, -1, -1, -1);
}

# Validate and apply one serialized command; the second result requests terminal shutdown.
handle(document: ref Document, request: Request): (Reply, int)
{
    if(request.id < 0)
        return (snapshot(document, request.id, Invalid), 0);
    if(request.operation < Open || request.operation > Redo)
        return (snapshot(document, request.id, Unknown), 0);
    if(request.operation == Snapshot)
        return (snapshot(document, request.id, Ok), 0);
    if(request.expected != document.revision)
        return (snapshot(document, request.id, Conflict), 0);
    if(request.operation == Shutdown)
        return (snapshot(document, request.id, Ok), 1);
    if(request.operation == Open){
        if(document.opened)
            return (snapshot(document, request.id, Invalid), 0);
        if(len request.text > MaxCharacters)
            return (snapshot(document, request.id, TooLarge), 0);
        if(document.revision == MaxRevision)
            return (snapshot(document, request.id, RevisionLimit), 0);
        document.revision++;
        document.opened = 1;
        document.saved = document.revision;
        document.text = request.text;
        document.history = ref History(nil, nil);
        return (snapshot(document, request.id, Ok), 0);
    }
    if(!document.opened)
        return (snapshot(document, request.id, Closed), 0);
    if(request.operation == Undo || request.operation == Redo)
        return (travel(document, request), 0);
    if(request.operation == Find)
        return (find(document, request), 0);
    if(request.operation == Locate || request.operation == Offset)
        return (coordinates(document, request), 0);
    if(request.operation == MarkSaved){
        document.saved = document.revision;
        return (snapshot(document, request.id, Ok), 0);
    }
    if(request.operation == Replace){
        if(request.first < 0 || request.last < request.first || request.last > len document.text)
            return (snapshot(document, request.id, RangeError), 0);
        remaining := len document.text - (request.last - request.first);
        if(len request.text > MaxCharacters - remaining)
            return (snapshot(document, request.id, TooLarge), 0);
        if(document.text[request.first:request.last] == request.text)
            return (snapshot(document, request.id, Ok), 0);
        if(document.revision == MaxRevision)
            return (snapshot(document, request.id, RevisionLimit), 0);
        # [GC:] Prepare text and bounded private history before publishing either.
        # Allocation traps belong to the runtime; no recovery guarantee is implied.
        text := document.text[:request.first] + request.text + document.text[request.last:];
        (_, undo) := pack(nil, document.text :: document.history.undo);
        history := ref History(undo, nil);
        document.text = text;
        document.history = history;
        document.revision++;
        return (snapshot(document, request.id, Ok), 0);
    }
    # Close retires the old session even when its text was already empty.
    if(document.revision == MaxRevision)
        return (snapshot(document, request.id, RevisionLimit), 0);
    document.revision++;
    document.opened = 0;
    document.saved = document.revision;
    document.text = "";
    document.history = ref History(nil, nil);
    return (snapshot(document, request.id, Ok), 0);
}

# Count logical snapshots/characters, including empty strings as entries.
# The current document and list/heap overhead are outside these logical budgets.
size(stack: list of string): (int, int)
{
    entries := 0;
    characters := 0;
    for(; stack != nil; stack = tl stack){
        entries++;
        characters += len hd stack;
    }
    return (entries, characters);
}

# Drop the farthest snapshot without changing existing persistent list nodes.
# [GC:] Rebuilt nodes are private; retained text values remain runtime-owned.
dropoldest(stack: list of string): list of string
{
    if(stack == nil)
        return nil;
    reverse: list of string;
    for(; tl stack != nil; stack = tl stack)
        reverse = hd stack :: reverse;
    result: list of string;
    for(; reverse != nil; reverse = tl reverse)
        result = hd reverse :: result;
    return result;
}

# Prefer the newly pushed inverse snapshot: evict farthest source entries first,
# then farthest target entries. Work is bounded by MaxHistoryEntries + one.
pack(source, target: list of string): (list of string, list of string)
{
    (se, sc) := size(source);
    (te, tc) := size(target);
    while(se + te > MaxHistoryEntries || sc + tc > MaxHistoryCharacters){
        if(source != nil){
            source = dropoldest(source);
            (se, sc) = size(source);
        }else{
            target = dropoldest(target);
            (te, tc) = size(target);
        }
    }
    return (source, target);
}

# Undo/Redo restores text as a NEW revision; old IDs never become valid again.
# saved remains the acknowledged disk revision, so text equality does not mark clean.
travel(document: ref Document, request: Request): Reply
{
    source := document.history.undo;
    target := document.history.redo;
    if(request.operation == Redo){
        source = document.history.redo;
        target = document.history.undo;
    }
    if(source == nil)
        return snapshot(document, request.id, NotFound);
    if(document.revision == MaxRevision)
        return snapshot(document, request.id, RevisionLimit);
    text := hd source;
    (source, target) = pack(tl source, document.text :: target);
    history := ref History(source, target);
    if(request.operation == Redo)
        history = ref History(target, source);
    # [GC:] Publish only completed text/history; old Reply values keep their text.
    document.text = text;
    document.history = history;
    document.revision++;
    return snapshot(document, request.id, Ok);
}

# Resolve coordinates against the serialized revision without changing any state.
# LF alone starts a line; CR remains a character so every offset round-trips,
# including the position between CR and LF. No UTF-8/UTF-16/display conversion.
coordinates(document: ref Document, request: Request): Reply
{
    result := snapshot(document, request.id, RangeError);
    offset := 0;
    line := 1;
    column := 0;
    if(request.operation == Locate){
        if(request.first < 0 || request.first > len document.text)
            return result;
        offset = request.first;
        for(index := 0; index < offset; index++){
            if(document.text[index] == '\n'){
                line++;
                column = 0;
            }else
                column++;
        }
    }else{
        if(request.first < 1 || request.last < 0)
            return result;
        while(line < request.first && offset < len document.text){
            if(document.text[offset] == '\n')
                line++;
            offset++;
        }
        if(line != request.first)
            return result;
        # Compare against the available line span before adding the column.
        end := offset;
        while(end < len document.text && document.text[end] != '\n')
            end++;
        if(request.last > end - offset)
            return result;
        column = request.last;
        offset += column;
    }
    result.status = Ok;
    result.offset = offset;
    result.line = line;
    result.column = column;
    return result;
}

# Search one revision's explicit half-open character range without wrapping.
# Prefix fallback gives linear time even for repeated near-matches; the table
# is bounded by the admitted document length and belongs only to this request.
find(document: ref Document, request: Request): Reply
{
    if(request.first < 0 || request.last < request.first || request.last > len document.text)
        return snapshot(document, request.id, RangeError);
    needle := request.text;
    if(len needle == 0)
        return snapshot(document, request.id, Invalid);
    if(len needle > request.last - request.first)
        return snapshot(document, request.id, NotFound);
    # [GC:] The owner roots document/needle and this private prefix table while
    # scanning. No array, mutable state or native pointer crosses a reply channel.
    prefix := array[len needle] of int;
    matched := 0;
    for(index := 1; index < len needle; index++){
        while(matched > 0 && needle[index] != needle[matched])
            matched = prefix[matched - 1];
        if(needle[index] == needle[matched])
            matched++;
        prefix[index] = matched;
    }
    matched = 0;
    for(index = request.first; index < request.last; index++){
        while(matched > 0 && document.text[index] != needle[matched])
            matched = prefix[matched - 1];
        if(document.text[index] == needle[matched])
            matched++;
        if(matched == len needle){
            location := request;
            location.operation = Locate;
            location.first = index + 1 - matched;
            return coordinates(document, location);
        }
    }
    return snapshot(document, request.id, NotFound);
}

# Own request ordering and reply backpressure; shutdown returns only after its acknowledgement.
serve(commands: chan of Request, document: ref Document)
{
    for(;;){
        request := <-commands;
        # [GC:] Channel values retain their own string/channel references. The owner keeps
        # the document rooted while waiting; replies never expose the mutable Document ADT.
        # A nil reply cannot be acknowledged, so it must not change or stop the service.
        if(request.reply == nil)
            continue;
        (reply, stop) := handle(document, request);
        request.reply <-= reply;
        if(stop)
            return;
    }
}
