implement Idedeskclip;
include "idedeskclip.m";


# Validate a borrowed Unicode value for bounded UTF-8 transfer. Return status and
# byte extent; reject NUL/surrogates/out-of-range scalars before conversion. No
# source, clipboard state, native handle or channel is mutated by validation.
validate(text: string): (int, int)
{
    if (len text > MaxBytes) {
        return (TooLarge, 0);
    }
    for (index := 0; index < len text; index++) {
        value := text[index];
        if (value <= 0 || value > 16r10ffff || (value >= 16rd800 && value <= 16rdfff)) {
            return (Invalid, 0);
        }
    }
    bytes := len array of byte text;
    if (bytes > MaxBytes) {
        return (TooLarge, bytes);
    }
    return (Ok, bytes);
}


# Own the immutable session value and generation. requests is exclusively served
# here; every non-nil reply receives a complete snapshot, including on rejection.
# Channel calls may block cooperatively. Terminal shutdown drops the retained
# text before replying; clients must never reuse the stopped endpoint.
serve(requests: chan of Request)
{
    text := "";
    bytes := 0;
    generation := 0;
    for (;;) {
        request := <-requests;
        if (request.reply == nil) {
            continue;
        }
        status := Invalid;
        stop := 0;
        if (request.id >= 0) {
            if (request.operation == Get) {
                status = Ok;
            } else if (request.operation == Shutdown) {
                text = "";
                bytes = 0;
                status = Ok;
                stop = 1;
            } else if (request.operation == Put || request.operation == Clear) {
                status = Conflict;
                if (request.expected == generation) {
                    candidate := request.text;
                    if (request.operation == Clear) {
                        candidate = "";
                    }
                    (checked, extent) := validate(candidate);
                    status = checked;
                    if (checked == Ok && candidate != text) {
                        status = Limit;
                        if (generation < 2147483647) {
                            text = candidate;
                            bytes = extent;
                            generation++;
                            status = Ok;
                        }
                    }
                }
            }
        }
        request.reply <-= Reply(request.id, status, generation, text, bytes);
        if (stop) {
            return;
        }
    }
}


# Create a fresh owner with empty text/generation zero. The returned channel is
# the only mutable capability; managed strings stay in Dis and never enter Flash.
start(): chan of Request
{
    requests := chan of Request;
    spawn serve(requests);
    return requests;
}
