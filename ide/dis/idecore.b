implement Idecore;
include "idecore.m";

# Private state belongs to one service process; it is never exposed as a mutable UI object.
Document: adt
{
    opened: int;       # Whether text represents an open document.
    revision: int;     # Never reset by close/reopen, so stale edits cannot acquire a new session.
    saved: int;        # The last explicitly acknowledged clean revision.
    text: string;      # Character-indexed document content, bounded by MaxCharacters.
};

# Create a typed channel boundary; no native UI or host-runtime call occurs here.
start(): chan of Request
{
    commands := chan of Request;
    document := ref Document(0, 0, 0, "");
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
    if(request.operation < Open || request.operation > Offset)
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
        return (snapshot(document, request.id, Ok), 0);
    }
    if(!document.opened)
        return (snapshot(document, request.id, Closed), 0);
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
        document.text = document.text[:request.first] + request.text + document.text[request.last:];
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
    return (snapshot(document, request.id, Ok), 0);
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
