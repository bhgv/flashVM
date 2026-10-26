implement Idedocprojection;
include "idecore.m";
include "idevalue.m";
include "ideprojection.m";
include "idedocprojection.m";


# Start a live document adapter. Loaded modules/channel are borrowed until explicit Release.
# epoch/identity are copied receiver identity; nil is returned for invalid admission inputs.
start(core: Idecore, values: Idevalue, document: chan of Idecore->Request,
      epoch: int, identity: string): chan of ref Ideprojection->Request
{
    if (core == nil || values == nil || document == nil || epoch <= 0
        || len identity == 0 || len identity > 256) {
        return nil;
    }
    endpoint := chan of ref Ideprojection->Request;
    spawn owner(core, values, document, epoch, identity, endpoint);
    return endpoint;
}


# Exchange one actual document request and consume the acknowledgement.
# document is borrowed; operation/expected/text are copied. A wait blocks only this adapter process.
# The caller must keep the document service alive until the adapter acknowledges Release.
exchange(document: chan of Idecore->Request, operation, expected, length: int,
         text: string): Idecore->Reply
{
    reply := chan of Idecore->Reply;
    document <-= Idecore->Request(1, operation, expected, 0, length, text, "", reply);
    return <-reply;
}


# Build a canonical numeric descriptor from one bounded document field.
# kind/number are copied; the returned record has no mutable document reference.
scalar(kind, number: int): ref Idevalue->Value
{
    return ref Idevalue->Value(kind, big number, 0.0, "", "", 0, "", nil, nil);
}


# Build one Object/Method descriptor, retaining its logical receiver and incarnation.
# identity/member are immutable values, not authority tokens; the adapter session authorizes them.
reference(kind, epoch: int, identity, member: string): ref Idevalue->Value
{
    return ref Idevalue->Value(kind, big 0, 0.0, "", identity, epoch, member, nil, nil);
}


# Project one declared stored field or method from an acknowledged document snapshot.
# state is borrowed immutable source data; unknown members return nil without evaluating source.
project(state: Idecore->Reply, member: string, epoch: int, identity: string): ref Idevalue->Value
{
    case member {
    "text" =>
        return ref Idevalue->Value(Idevalue->Text, big 0, 0.0, state.text, "", 0, "", nil, nil);
    "length" =>
        return scalar(Idevalue->Int, len state.text);
    "revision" =>
        return scalar(Idevalue->Int, state.revision);
    "saved" =>
        return scalar(Idevalue->Int, state.saved);
    "dirty" =>
        return scalar(Idevalue->Bool, state.revision != state.saved);
    "self" =>
        return reference(Idevalue->Object, epoch, identity, "");
    "undo" or "redo" =>
        return reference(Idevalue->Method, epoch, identity, member);
    }
    return nil;
}


# Map a real document rejection into the shared projection status vocabulary.
# status is an Idecore result; return value never reports a failed mutation as successful.
status(status: int): int
{
    case status {
    Idecore->Ok =>
        return Ideprojection->Ok;
    Idecore->Conflict or Idecore->Closed =>
        return Ideprojection->Stale;
    Idecore->RevisionLimit =>
        return Ideprojection->Denied;
    }
    return Ideprojection->Invalid;
}


# Serve copied reads and explicit revision-checked mutations against the actual document owner.
# core/values/document are borrowed; epoch/identity bind this receiver; endpoint owns adapter requests.
# Reply waits block this process only. Pause prevents this adapter's mutations, not other editor clients.
owner(core: Idecore, values: Idevalue, document: chan of Idecore->Request,
      epoch: int, identity: string, endpoint: chan of ref Ideprojection->Request)
{
    paused := 0;
    revision := 0;
    for (;;) {
        request := <-endpoint;
        if (request == nil || request.reply == nil) {
            continue;
        }
        response := Ideprojection->Reply(request.id, request.site, Ideprojection->Invalid,
                                         epoch, revision, 0, "", nil);
        if (request.id < 0 || request.site < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.epoch != epoch) {
            response.status = Ideprojection->Stale;
            request.reply <-= response;
            continue;
        }
        if (request.operation == Ideprojection->Release) {
            response.status = Ideprojection->Ok;
            request.reply <-= response;
            return; # Retire borrowing without shutting down the actual document.
        }
        state := exchange(document, core->Snapshot, 0, 0, "");
        revision = state.revision;
        response.revision = revision;
        if (!state.opened || state.status != core->Ok) {
            response.status = Ideprojection->Stale;
            request.reply <-= response;
            continue;
        }
        case request.operation {
        Ideprojection->Describe =>
            response.status = Ideprojection->Ok;
            response.text = "text:string;length:int;revision:int;saved:int;dirty:bool;undo(expected:int);redo(expected:int)";
        Ideprojection->ReadAs =>
            response.value = values->convert(project(state, request.member, epoch, identity), request.asType);
            if (response.value != nil) {
                response.status = Ideprojection->Ok;
            }
        Ideprojection->ToTable =>
            # Metadata stays bounded even for a large document; full text requires an explicit ReadAs.
            names := array[] of {"length", "revision", "saved", "dirty", "self"};
            fields := array[len names] of ref Idevalue->Value;
            for (i := 0; i < len names; i++) {
                fields[i] = project(state, names[i], epoch, identity);
            }
            response.value = values->copy(ref Idevalue->Value(Idevalue->Table, big 0, 0.0, "", "", 0, "", names, fields));
            if (response.value != nil) {
                response.status = Ideprojection->Ok;
            }
        Ideprojection->WriteValue or Ideprojection->InvokeRef =>
            if (paused) {
                response.status = Ideprojection->Paused;
            } else if (request.number != state.revision) {
                response.status = Ideprojection->Stale;
            } else {
                argument := values->copy(request.value);
                if (request.operation == Ideprojection->WriteValue) {
                    if (request.member == "text" && argument != nil && argument.kind == Idevalue->Text) {
                        state = exchange(document, core->Replace, request.number, len state.text, argument.text);
                        response.status = status(state.status);
                    }
                } else if (argument != nil && argument.kind == Idevalue->Method) {
                    if (argument.owner != identity || argument.epoch != epoch) {
                        response.status = Ideprojection->Stale;
                    } else if (request.permit != 1) {
                        response.status = Ideprojection->Denied;
                    } else if (argument.member == "undo" || argument.member == "redo") {
                        operation := core->Undo;
                        if (argument.member == "redo") {
                            operation = core->Redo;
                        }
                        state = exchange(document, operation, request.number, 0, "");
                        response.status = status(state.status);
                    }
                }
                revision = state.revision;
                response.revision = revision;
                if (response.status == Ideprojection->Ok) {
                    response.value = reference(Idevalue->Object, epoch, identity, "");
                }
            }
        Ideprojection->Pause =>
            paused = 1;
            response.status = Ideprojection->Ok;
        Ideprojection->Resume =>
            paused = 0;
            response.status = Ideprojection->Ok;
        }
        request.reply <-= response;
    }
}
