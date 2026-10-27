implement Idefiles;
include "sys.m";
include "idefiles.m";


# Validate a bounded Unicode native path before any open.
# path is borrowed and immutable. Returns one for an admitted name, zero otherwise.
validpath(path: string): int
{
    if (len path == 0 || len path > MaxPathBytes || len array of byte path > MaxPathBytes) {
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


# Find a live owner-local cursor. identities is private to this service process;
# cursor is a copied capability component. Return its slot or -1 without I/O.
findcursor(identities: array of big, cursor: big): int
{
    if (cursor <= big 0) {
        return -1;
    }
    for (index := 0; index < len identities; index++) {
        if (identities[index] == cursor) {
            return index;
        }
    }
    return -1;
}


# Retire a private cursor and its FD before a reply can be published.
# The slot is already checked by the owner; arrays never escape this process.
# [GC:] Clearing the final FD reference closes the native resource synchronously.
retirecursor(identities: array of big, handles: array of ref Sys->FD,
             exhausted: array of int, slot: int)
{
    handles[slot] = nil;
    identities[slot] = big 0;
    exhausted[slot] = 0;
}


# Serve metadata and bounded directory traversal using actual hosted Sys calls.
# sys/commands remain borrowed until Shutdown; private arrays own all live FDs.
# Replies transfer independent records, never expose native handles or retain
# caller-mutable batches. Every admitted reply is a blocking rendezvous.
# Native I/O can block the VM owner; no background worker/cancellation is implied.
servedirectories(sys: Sys, commands: chan of DirectoryRequest)
{
    # Scalar array cells are unspecified without an initializer in Limbo.
    identities := array[MaxDirectories] of {* => big 0};
    handles := array[MaxDirectories] of ref Sys->FD;
    exhausted := array[MaxDirectories] of {* => 0};
    sequence := big 0;
    maximum := big 9223372036854775807;
    for (;;) {
        request := <-commands;
        if (request.reply == nil) {
            continue;
        }
        response := DirectoryReply(request.id, Invalid, request.cursor, Sys->zerodir,
                                   nil, "invalid directory request");
        if (request.id < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.operation == Shutdown) {
            for (index := 0; index < MaxDirectories; index++) {
                retirecursor(identities, handles, exhausted, index);
            }
            response.status = Ok;
            response.cursor = big 0;
            response.message = "";
            request.reply <-= response;
            return;
        }
        if (request.operation == OpenDir || request.operation == StatPath) {
            if (request.cursor != big 0 || !validpath(request.path)) {
                request.reply <-= response;
                continue;
            }
            if (request.operation == StatPath) {
                (status, metadata) := sys->stat(request.path);
                if (status < 0) {
                    response.status = Failed;
                    response.message = "stat: " + sys->sprint("%r");
                } else {
                    response.status = Ok;
                    response.metadata = metadata;
                    response.message = "";
                }
                request.reply <-= response;
                continue;
            }
            slot := -1;
            for (index := 0; index < MaxDirectories; index++) {
                if (identities[index] == big 0) {
                    slot = index;
                    break;
                }
            }
            if (slot < 0 || sequence == maximum) {
                response.status = Full;
                response.message = "directory cursor quota or identity space exhausted";
                request.reply <-= response;
                continue;
            }
            file := sys->open(request.path, Sys->OREAD);
            if (file == nil) {
                response.status = Failed;
                response.message = "open directory: " + sys->sprint("%r");
                request.reply <-= response;
                continue;
            }
            (status, metadata) := sys->fstat(file);
            if (status < 0 || (metadata.mode & Sys->DMDIR) == 0) {
                response.status = Failed;
                response.message = "opened path is not an accessible directory";
                file = nil;
                request.reply <-= response;
                continue;
            }
            sequence++;
            identities[slot] = sequence;
            handles[slot] = file;
            file = nil; # [GC:] Transfer the sole service FD retain into the private slot.
            exhausted[slot] = 0;
            response.status = Ok;
            response.cursor = sequence;
            response.metadata = metadata;
            response.message = "";
            request.reply <-= response;
            continue;
        }
        if (request.operation != NextDir && request.operation != CloseDir) {
            response.message = "unknown directory operation";
            request.reply <-= response;
            continue;
        }
        slot := findcursor(identities, request.cursor);
        if (slot < 0) {
            response.status = Stale;
            response.message = "directory cursor is closed or unknown to this endpoint";
            request.reply <-= response;
            continue;
        }
        if (request.operation == CloseDir) {
            retirecursor(identities, handles, exhausted, slot);
            response.status = Ok;
            response.message = "";
            request.reply <-= response;
            continue;
        }
        if (exhausted[slot]) {
            response.status = End;
            response.message = "";
            request.reply <-= response;
            continue;
        }
        (count, entries) := sys->dirread(handles[slot]);
        if (count < 0 || count != len entries || count > MaxBatch) {
            response.status = Failed;
            response.message = "dirread: " + sys->sprint("%r");
            retirecursor(identities, handles, exhausted, slot);
        } else if (count == 0) {
            exhausted[slot] = 1;
            handles[slot] = nil; # [GC:] Release the FD at EOF but retain the explicit cursor identity.
            response.status = End;
            response.message = "";
        } else {
            response.status = Ok;
            response.entries = entries;
            response.message = "";
        }
        request.reply <-= response;
    }
}


# Start a new owner-local cursor namespace. sys is borrowed and must be initialized.
# Return nil for a nil module, otherwise a blocking typed request endpoint.
# Caller must consume replies and explicitly Shutdown to retire open resources.
startdirs(sys: Sys): chan of DirectoryRequest
{
    if (sys == nil) {
        return nil;
    }
    commands := chan of DirectoryRequest;
    spawn servedirectories(sys, commands);
    return commands;
}
