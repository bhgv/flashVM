implement Ideprojection;
include "idevalue.m";
include "ideprojection.m";


# Allocate a typed owner endpoint. epoch is an incarnation, not an object address.
# Returns nil for invalid input; a valid service blocks only in its own Dis process.
start(epoch: int): chan of ref Request
{
    return startnamed(epoch, "sample-root");
}


# Start an explicitly bound receiver. identity is copied and must come from the adapter registry.
# Returns nil for invalid epoch/name/module. No caller target reference is retained.
startnamed(epoch: int, identity: string): chan of ref Request
{
    if (epoch <= 0 || len identity == 0 || len identity > 256) {
        return nil;
    }
    values := load Idevalue Idevalue->PATH;
    if (values == nil) {
        return nil;
    }
    requests := chan of ref Request;
    spawn owner(requests, epoch, identity, values);
    return requests;
}


# Own the demonstrator object and serialize operations. requests is borrowed for this lifetime.
# epoch/identity bind this receiver; values is a borrowed loaded pure conversion module.
# Never dereference caller heap pointers; Limbo channels carry typed records within this VM.
# Reply rendezvous can block this owner, so the caller must consume every admitted reply.
# Pause is cooperative target state, not a process-wide debugger stop or native RPC adapter.
owner(requests: chan of ref Request, epoch: int, identity: string, values: Idevalue)
{
    number := 7;
    text := "inspectable";
    revision := 0;
    paused := 0;
    for (;;) {
        request := <-requests;
        if (request == nil || request.reply == nil) {
            continue;
        }
        response := Reply(request.id, request.site, Invalid, epoch, revision, 0, "", nil);
        stop := 0;
        if (request.id < 0 || request.site < 0) {
            request.reply <-= response;
            continue;
        }
        if (request.epoch != epoch) {
            response.status = Stale;
        } else {
            case request.operation {
            Describe =>
                response.status = Ok;
                response.text = "number:int;text:string;add(int):int";
            Read =>
                if (request.member == "number") {
                    response.status = Ok;
                    response.number = number;
                } else if (request.member == "text") {
                    response.status = Ok;
                    response.text = text;
                }
            Convert =>
                response.value = values->convert(request.value, request.asType);
                if (response.value != nil) {
                    response.status = Ok;
                }
            WriteValue =>
                copied := values->copy(request.value);
                if (paused) {
                    response.status = Paused;
                } else if (revision == 16r7fffffff) {
                    response.status = Denied;
                } else if (copied != nil) {
                    if (request.member == "number" && copied.kind == Idevalue->Int) {
                        number = int copied.integer;
                        revision++;
                        response.status = Ok;
                        response.value = copied;
                    } else if (request.member == "text" && copied.kind == Idevalue->Text) {
                        text = copied.text;
                        revision++;
                        response.status = Ok;
                        response.value = copied;
                    }
                }
            ReadAs =>
                source: ref Idevalue->Value;
                case request.member {
                "number" =>
                    source = ref Idevalue->Value(Idevalue->Int, big number, 0.0, "", "", 0, "", nil, nil);
                "text" =>
                    source = ref Idevalue->Value(Idevalue->Text, big 0, 0.0, text, "", 0, "", nil, nil);
                "self" =>
                    source = ref Idevalue->Value(Idevalue->Object, big 0, 0.0, "", identity, epoch, "", nil, nil);
                "add" =>
                    source = ref Idevalue->Value(Idevalue->Method, big 0, 0.0, "", identity, epoch, "add", nil, nil);
                }
                response.value = values->convert(source, request.asType);
                if (response.value != nil) {
                    response.status = Ok;
                }
            ToTable =>
                # An explicit snapshot is data, not an executable clone of this receiver.
                # Copy only declared stored fields; never invoke getters or retain a method body.
                names := array[] of {"number", "text", "self"};
                fields := array[] of {
                    ref Idevalue->Value(Idevalue->Int, big number, 0.0, "", "", 0, "", nil, nil),
                    ref Idevalue->Value(Idevalue->Text, big 0, 0.0, text, "", 0, "", nil, nil),
                    ref Idevalue->Value(Idevalue->Object, big 0, 0.0, "", identity, epoch, "", nil, nil)
                };
                response.value = values->copy(ref Idevalue->Value(Idevalue->Table, big 0, 0.0, "", "", 0, "", names, fields));
                if (response.value != nil) {
                    response.status = Ok;
                }
            InvokeRef =>
                method := values->copy(request.value);
                if (method == nil || method.kind != Idevalue->Method) {
                    response.status = Invalid;
                } else if (method.owner != identity || method.epoch != epoch) {
                    response.status = Stale;
                } else if (request.permit != 1) {
                    response.status = Denied;
                } else if (paused) {
                    response.status = Paused;
                } else if (method.member == "add" && revision != 16r7fffffff) {
                    number += request.number;
                    revision++;
                    response.status = Ok;
                    response.number = number;
                    response.value = ref Idevalue->Value(Idevalue->Int, big number, 0.0, "", "", 0, "", nil, nil);
                }
            Write =>
                if (paused) {
                    response.status = Paused;
                } else if (revision == 16r7fffffff) {
                    response.status = Denied;
                } else if (request.member == "number") {
                    number = request.number;
                    revision++;
                    response.status = Ok;
                } else if (request.member == "text" && len request.text <= MaxText) {
                    text = request.text;
                    revision++;
                    response.status = Ok;
                }
            Invoke =>
                if (request.permit != 1) {
                    response.status = Denied;
                } else if (paused) {
                    response.status = Paused;
                } else if (request.member == "add" && revision != 16r7fffffff) {
                    # This explicit method is the only executable target operation.
                    # Integer addition follows the selected Dis target's integer semantics.
                    number += request.number;
                    revision++;
                    response.number = number;
                    response.status = Ok;
                }
            Pause =>
                paused = 1;
                response.status = Ok;
            Resume =>
                paused = 0;
                response.status = Ok;
            Release =>
                response.status = Ok;
                stop = 1;
            }
        }
        response.revision = revision;
        request.reply <-= response;
        if (stop) {
            return;
        }
    }
}
