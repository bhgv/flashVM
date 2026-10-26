implement Ideprojection;
include "ideprojection.m";


# Allocate a typed owner endpoint. epoch is an incarnation, not an object address.
# Returns nil for invalid input; a valid service blocks only in its own Dis process.
start(epoch: int): chan of ref Request
{
    if(epoch <= 0){
        return nil;
    }
    requests := chan of ref Request;
    spawn owner(requests, epoch);
    return requests;
}


# Own the demonstrator object and serialize operations. requests is borrowed for this lifetime.
# Never dereference caller heap pointers; Limbo channels carry typed records within this VM.
# Reply rendezvous can block this owner, so the caller must consume every admitted reply.
# Pause is cooperative target state, not a process-wide debugger stop or native RPC adapter.
owner(requests: chan of ref Request, epoch: int)
{
    number := 7;
    text := "inspectable";
    revision := 0;
    paused := 0;
    for(;;){
        request := <-requests;
        if(request == nil || request.reply == nil){
            continue;
        }
        response := Reply(request.id, request.site, Invalid, epoch, revision, 0, "");
        stop := 0;
        if(request.id < 0 || request.site < 0){
            request.reply <-= response;
            continue;
        }
        if(request.epoch != epoch){
            response.status = Stale;
        }else{
            case request.operation {
            Describe =>
                response.status = Ok;
                response.text = "number:int;text:string;add(int):int";
            Read =>
                if(request.member == "number"){
                    response.status = Ok;
                    response.number = number;
                }else if(request.member == "text"){
                    response.status = Ok;
                    response.text = text;
                }
            Write =>
                if(paused){
                    response.status = Paused;
                }else if(revision == 16r7fffffff){
                    response.status = Denied;
                }else if(request.member == "number"){
                    number = request.number;
                    revision++;
                    response.status = Ok;
                }else if(request.member == "text" && len request.text <= MaxText){
                    text = request.text;
                    revision++;
                    response.status = Ok;
                }
            Invoke =>
                if(request.permit != 1){
                    response.status = Denied;
                }else if(paused){
                    response.status = Paused;
                }else if(request.member == "add" && revision != 16r7fffffff){
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
        if(stop){
            return;
        }
    }
}
