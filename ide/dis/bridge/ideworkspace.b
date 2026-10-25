implement Ideworkspace;
include "idecore.m";
include "ideworkspace.m";


# Private document state; mutable references are never returned to clients.
Document: adt
{
    entry: ref Entry;                    # Owned current metadata.
    commands: chan of Idecore->Request;  # Private document/history owner endpoint.
    pending: int;                        # Most recent active compile ticket, zero when none.
    base: int;                           # Source revision pinned by pending.
    target: string;                      # Target of the pending/successful build.
    built: int;                          # Revision of the retained successful artifact.
    builtticket: int;                    # Ticket of the retained successful artifact.
    builttarget: string;                 # Format of retained bytes, independent of a newer pending job.
    bytes: array of byte;                # Owned retained binary, never exposed directly.
    diagnostic: string;                  # Diagnostic text belonging to retained successful bytes.
};


# Return whether text contains only Unicode scalars. value is borrowed; no mutation occurs.
validtext(value: string): int
{
    for (i := 0; i < len value; i++) {
        c := value[i];
        if (c < 0 || c > 16r10ffff || (c >= 16rd800 && c <= 16rdfff)) {
            return 0;
        }
    }
    return 1;
}


# Check an absolute logical source path, with at most 255 encoded UTF-8 bytes.
# path is borrowed. Returns one on canonical segments, zero on invalid syntax.
validpath(path: string): int
{
    if (len path < 2 || path[0] != '/' || !validtext(path) || len array of byte path > 255) {
        return 0;
    }
    first := 1;
    for (i := 1; i <= len path; i++) {
        if (i == len path || path[i] == '/') {
            part := path[first:i];
            if (part == "" || part == "." || part == "..") {
                return 0;
            }
            first = i + 1;
        } else if (path[i] < 32 || path[i] == 127 || path[i] == '\\' || path[i] == ':') {
            return 0;
        }
    }
    return 1;
}


# Perform one synchronous operation on a private document owner.
# document borrows its endpoint; request is copied and reply privately replaced.
# Returns the complete core result, keeping the public metadata current.
call(document: ref Document, request: Idecore->Request): Idecore->Reply
{
    response := chan of Idecore->Reply;
    request.reply = response;
    document.commands <-= request;
    result := <-response;
    document.entry.revision = result.revision;
    document.entry.saved = result.saved;
    return result;
}


# Read a complete document snapshot without an expected revision; return its result.
snapshot(document: ref Document): Idecore->Reply
{
    request := Idecore->Request(1, Idecore->Snapshot, 0, 0, 0, "", "", nil);
    return call(document, request);
}


# Stop a private owner after workspace policy has approved removal; returns nothing.
stop(document: ref Document)
{
    request := Idecore->Request(1, Idecore->Shutdown, document.entry.revision, 0, 0, "", "", nil);
    call(document, request);
}


# Clone bytes so an external worker/client cannot mutate retained artifact state.
# bytes is borrowed; returns an independently allocated array of identical extent.
copybytes(bytes: array of byte): array of byte
{
    result := array[len bytes] of byte;
    result[0:] = bytes;
    return result;
}


# Serialize document lookup, edit forwarding and revision-bound build publication.
# core is a loaded document implementation; commands is the owned request endpoint.
# Returns only after an acknowledged valid Shutdown; no Sys/Draw module is loaded.
serve(core: Idecore, commands: chan of Request)
{
    documents := array[MaxDocuments] of ref Document;
    nextdocument := 0;
    nextticket := 0;
    totalbytes := 0;
    for (;;) {
        request := <-commands;
        if (request.reply == nil) {
            continue;
        }
        empty := Idecore->Reply(0, Idecore->Closed, 0, 0, 0, "", -1, -1, -1, -1, -1);
        result := Reply(request.id, Invalid, 0, empty, nil, 0, "", "", nil);
        slot := -1;
        free := -1;
        for (i := 0; i < len documents; i++) {
            if (documents[i] == nil) {
                free = i;
            } else if (documents[i].entry.document == request.document) {
                slot = i;
            }
        }
        if (request.id < 0) {
            request.reply <-= result;
            continue;
        }
        if (request.operation == Open) {
            if (!validpath(request.path)) {
                request.reply <-= result;
                continue;
            }
            duplicate := 0;
            for (i = 0; i < len documents; i++) {
                if (documents[i] != nil && documents[i].entry.path == request.path) {
                    duplicate = 1;
                }
            }
            if (duplicate) {
                result.status = Conflict;
            } else if (free < 0 || nextdocument == Idecore->MaxRevision) {
                result.status = Limit;
            } else {
                document := ref Document(nil, nil, 0, 0, "", 0, 0, "", nil, "");
                document.entry = ref Entry(++nextdocument, request.path, 0, 0);
                document.commands = core->start();
                opening := Idecore->Request(1, Idecore->Open, 0, 0, 0, request.text, "", nil);
                result.core = call(document, opening);
                if (result.core.status == Idecore->Ok) {
                    documents[free] = document;
                    result.status = Ok;
                    result.document = document.entry.document;
                } else {
                    stop(document);
                    result.status = Invalid;
                }
            }
        } else if (request.operation == List) {
            count := 0;
            for (i = 0; i < len documents; i++) {
                if (documents[i] != nil) {
                    count++;
                }
            }
            result.entries = array[count] of ref Entry;
            at := 0;
            for (i = 0; i < len documents; i++) {
                if (documents[i] != nil) {
                    entry := documents[i].entry;
                    result.entries[at++] = ref Entry(entry.document, entry.path, entry.revision, entry.saved);
                }
            }
            result.status = Ok;
        } else if (request.operation == Shutdown) {
            result.status = Ok;
            if (request.force != 0 && request.force != 1) {
                result.status = Invalid;
            } else if (!request.force) {
                for (i = 0; i < len documents; i++) {
                    if (documents[i] != nil && documents[i].entry.revision != documents[i].entry.saved) {
                        result.status = Dirty;
                    }
                }
            }
            if (result.status == Ok) {
                for (i = 0; i < len documents; i++) {
                    if (documents[i] != nil) {
                        stop(documents[i]);
                    }
                }
                request.reply <-= result;
                return;
            }
        } else if (slot < 0) {
            result.status = NotFound;
        } else {
            document := documents[slot];
            result.document = document.entry.document;
            result.core = snapshot(document);
            if (request.operation == Command) {
                operation := request.command.operation;
                if (operation != Idecore->Open && operation != Idecore->Close && operation != Idecore->Shutdown) {
                    result.core = call(document, request.command);
                    result.status = Ok;
                }
            } else if (request.operation == BeginBuild) {
                if (request.revision != result.core.revision) {
                    result.status = Conflict;
                } else if (request.target != "dis" && request.target != "swf" && request.target != "fir") {
                    result.status = Invalid;
                } else if (nextticket == Idecore->MaxRevision) {
                    result.status = Limit;
                } else {
                    document.pending = ++nextticket;
                    document.base = result.core.revision;
                    document.target = request.target;
                    result.ticket = document.pending;
                    result.target = document.target;
                    result.text = result.core.text;
                    result.status = Ok;
                }
            } else if (request.operation == FinishBuild) {
                if (request.ticket <= 0 || request.ticket != document.pending ||
                    request.revision != document.base || request.revision != result.core.revision) {
                    result.status = Conflict;
                } else if ((request.success != 0 && request.success != 1) ||
                           !validtext(request.text) || len request.text > MaxDiagnosticCharacters) {
                    result.status = Invalid;
                } else if (!request.success) {
                    document.pending = 0;
                    result.status = BuildFailed;
                    result.text = request.text;
                } else if (len request.artifact == 0 || len request.artifact > MaxArtifactBytes ||
                           len request.artifact > MaxTotalArtifactBytes - (totalbytes - len document.bytes)) {
                    result.status = Limit;
                } else {
                    bytes := copybytes(request.artifact);
                    totalbytes += len bytes - len document.bytes;
                    document.bytes = bytes;
                    document.built = document.base;
                    document.builtticket = request.ticket;
                    document.builttarget = document.target;
                    document.pending = 0;
                    document.diagnostic = request.text;
                    result.ticket = request.ticket;
                    result.target = document.target;
                    result.text = document.diagnostic;
                    result.status = Ok;
                }
            } else if (request.operation == Artifact) {
                if (len document.bytes == 0) {
                    result.status = NotFound;
                } else if (document.built != result.core.revision) {
                    result.status = Conflict;
                } else {
                    result.artifact = copybytes(document.bytes);
                    result.ticket = document.builtticket;
                    result.target = document.builttarget;
                    result.text = document.diagnostic;
                    result.status = Ok;
                }
            } else if (request.operation == Close) {
                if (request.revision != result.core.revision) {
                    result.status = Conflict;
                } else if (request.force != 0 && request.force != 1) {
                    result.status = Invalid;
                } else if (!request.force && result.core.revision != result.core.saved) {
                    result.status = Dirty;
                } else {
                    stop(document);
                    totalbytes -= len document.bytes;
                    documents[slot] = nil;
                    result.status = Ok;
                }
            }
        }
        request.reply <-= result;
    }
}


# Start one workspace owner. core is a loaded implementation; return nil if absent.
start(core: Idecore): chan of Request
{
    if (core == nil) {
        return nil;
    }
    commands := chan of Request;
    spawn serve(core, commands);
    return commands;
}
