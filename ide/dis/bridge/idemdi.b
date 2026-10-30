implement Idemdi;
include "idemdi.m";


# Return an independent integer-array copy; input is borrowed immutable state.
# No allocation/reference is shared with the private registry after publication.
copy(values: array of int): array of int
{
    result := array[len values] of int;
    result[0:] = values;
    return result;
}


# Own one desktop's MDI/SDI membership. Validate the complete command before
# mutation. Source and graphics containers cannot mix tool families. Every
# accepted terminal Shutdown is acknowledged before this sole owner exits.
serve(requests: chan of Request)
{
    families := array[Limit] of int;
    containers := array[Limit] of int;
    tools := array[Limit] of int;
    for (index := 0; index < Limit; index++) {
        families[index] = 0;
        containers[index] = 0;
        tools[index] = 0;
    }
    revision := 1;
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Invalid;
        changed := 0;
        stop := 0;
        document := request.document - 1;
        container := request.container - 1;
        if (request.id >= 0 && request.operation >= Snapshot && request.operation <= Shutdown) {
            if (request.operation == Snapshot) {
                status = Ok;
            } else if (request.revision != revision) {
                status = Conflict;
            } else if (request.operation == Shutdown) {
                status = Ok;
                stop = 1;
            } else if (request.operation == Register && document >= 0 && document < Limit &&
                       (request.family == Source || request.family == Graphics) && families[document] == 0) {
                families[document] = request.family;
                changed = 1;
                status = Ok;
            } else if (request.operation == Create && container >= 0 && container < Limit &&
                       (request.family == Source || request.family == Graphics) && tools[container] == 0) {
                tools[container] = request.family;
                changed = 1;
                status = Ok;
            } else if (request.operation == Join && document >= 0 && document < Limit &&
                       container >= 0 && container < Limit && families[document] != 0 &&
                       tools[container] == families[document]) {
                changed = containers[document] != request.container;
                containers[document] = request.container;
                status = Ok;
            } else if ((request.operation == Leave || request.operation == Remove) &&
                       document >= 0 && document < Limit && families[document] != 0) {
                changed = containers[document] != 0 || request.operation == Remove;
                containers[document] = 0;
                if (request.operation == Remove) {
                    families[document] = 0;
                }
                status = Ok;
            } else if (request.operation == Dissolve && container >= 0 && container < Limit && tools[container] != 0) {
                for (index := 0; index < Limit; index++) {
                    if (containers[index] == request.container) {
                        containers[index] = 0;
                    }
                }
                tools[container] = 0;
                changed = 1;
                status = Ok;
            }
        }
        if (changed) {
            if (revision == 2147483647) {
                raise "fail: MDI membership revision exhausted";
            }
            revision++;
        }
        request.reply <-= Reply(request.id, revision, status, copy(families), copy(containers), copy(tools));
        if (stop) {
            return;
        }
    }
}


# Create the sole membership owner; documents/renderers are neither loaded nor
# borrowed. Caller owns the returned channel and must acknowledge Shutdown.
start(): chan of Request
{
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}
