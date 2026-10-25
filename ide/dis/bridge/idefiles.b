implement Idefiles;
include "sys.m";
include "idefiles.m";


# Validate a bounded Unicode native path before any open.
# path is borrowed and immutable. Returns one for an admitted name, zero otherwise.
validpath(path: string): int
{
    if (len path == 0 || len array of byte path > MaxPathBytes) {
        return 0;
    }
    for (index := 0; index < len path; index++) {
        value := path[index];
        if (value <= 0 || value > 16r10ffff || (value >= 16rd800 && value <= 16rdfff)) {
            return 0;
        }
    }
    return 1;
}


# Read a bounded binary window through actual Sys open/seek/readn operations.
# sys/request are borrowed until completion. Returns independently owned bytes,
# closes the descriptor before returning, and never changes a document or file.
readwindow(sys: Sys, request: Request): Reply
{
    result := Reply(request.id, Invalid, big -1, big -1, nil, "invalid file-window request");
    if (request.id < 0 || request.count < 0 || request.count > MaxBytes ||
        request.origin < Sys->SEEKSTART || request.origin > Sys->SEEKEND || !validpath(request.path)) {
        return result;
    }
    result.status = Failed;
    file := sys->open(request.path, Sys->OREAD);
    if (file == nil) {
        result.message = "cannot open file";
        return result;
    }
    extent := sys->seek(file, big 0, Sys->SEEKEND);
    # Restore the fresh-handle cursor before honoring SEEKRELA semantics.
    if (extent < big 0 || sys->seek(file, big 0, Sys->SEEKSTART) != big 0) {
        file = nil;
        result.message = "cannot measure file";
        return result;
    }
    position := sys->seek(file, request.offset, request.origin);
    if (position < big 0) {
        file = nil;
        result.message = "cannot seek file";
        return result;
    }
    bytes := array[request.count] of byte;
    count := sys->readn(file, bytes, len bytes);
    file = nil;
    if (count < 0 || count > len bytes) {
        result.message = "cannot read file";
        return result;
    }
    result.status = Ok;
    result.size = extent;
    result.position = position;
    result.bytes = bytes[:count];
    result.message = "";
    return result;
}


# Serialize requests and transfer each result through its caller-owned reply endpoint.
# sys and commands remain borrowed by this process until acknowledged Shutdown.
# Returns nothing; nil reply endpoints are ignored, avoiding an unanswerable request.
serve(sys: Sys, commands: chan of Request)
{
    for (;;) {
        request := <-commands;
        if (request.reply == nil) {
            continue;
        }
        if (request.operation == Shutdown && request.id >= 0) {
            request.reply <-= Reply(request.id, Ok, big -1, big -1, nil, "");
            return;
        }
        if (request.operation != Read) {
            request.reply <-= Reply(request.id, Invalid, big -1, big -1, nil, "unknown file-window operation");
            continue;
        }
        request.reply <-= readwindow(sys, request);
    }
}


# Start one independent file-window owner using a borrowed initialized Sys module.
# Returns nil if sys is nil; otherwise returns the blocking request channel.
start(sys: Sys): chan of Request
{
    if (sys == nil) {
        return nil;
    }
    commands := chan of Request;
    spawn serve(sys, commands);
    return commands;
}
