implement Ideinspect;
include "idevalue.m";
include "ideprojection.m";
include "ideinspect.m";


# Allocate a channel-only model owner. Returns its live request endpoint.
# [GC:] Row state belongs to the spawned process; Shutdown acknowledges before exit.
start(): chan of ref Request
{
    values := load Idevalue Idevalue->PATH;
    if (values == nil) {
        return nil;
    }
    requests := chan of ref Request;
    spawn owner(requests, values);
    return requests;
}


# Copy an initialized row. value is borrowed/non-nil; result has no mutable record alias.
copy(value: ref Row, values: Idevalue): ref Row
{
    return ref Row(value.epoch, value.member, value.ticket, value.pending,
                   value.status, value.revision, value.number, value.text, values->copy(value.value));
}


# Serialize watch-row transitions without waiting on a target or a transport worker.
# requests is the owned endpoint. A client must consume its reply to avoid rendezvous blocking.
# A monotonically increasing ticket fences late replies after selection changes or refreshes.
owner(requests: chan of ref Request, values: Idevalue)
{
    rows := array[MaxRows] of ref Row;
    serial := 0;
    for (;;) {
        request := <-requests;
        if (request == nil || request.reply == nil) {
            continue;
        }
        if (request.operation == Shutdown) {
            request.reply <-= Reply(Ok, nil);
            return;
        }
        if (request.row < 0 || request.row >= MaxRows) {
            request.reply <-= Reply(Invalid, nil);
            continue;
        }
        row := rows[request.row];
        status := Invalid;
        case request.operation {
        Select =>
            if (request.epoch > 0 && len request.member <= 256) {
                if (serial == 16r7fffffff) {
                    status = Limit;
                } else {
                    serial++;
                    row = ref Row(request.epoch, request.member, serial, 0,
                                  Ideprojection->Invalid, 0, 0, "", nil);
                    rows[request.row] = row;
                    status = Ok;
                }
            }
        Begin =>
            if (row != nil) {
                if (serial == 16r7fffffff) {
                    status = Limit;
                } else {
                    serial++;
                    row.ticket = serial;
                    row.pending = 1;
                    status = Ok;
                }
            }
        Apply =>
            result := request.result;
            if (row != nil && result != nil) {
                status = Stale;
                if (row.pending && request.ticket == row.ticket && result.id == row.ticket
                   && result.epoch == row.epoch && result.revision >= row.revision) {
                    if (result.status >= Ideprojection->Ok && result.status <= Ideprojection->Denied
                       && len result.text <= Ideprojection->MaxText) {
                        copied := values->copy(result.value);
                        if (result.value != nil && copied == nil) {
                            request.reply <-= Reply(Invalid, nil);
                            continue;
                        }
                        row.value = copied;
                        row.status = result.status;
                        row.revision = result.revision;
                        row.number = result.number;
                        row.text = result.text;
                        row.pending = 0;
                        status = Ok;
                    } else {
                        status = Invalid;
                    }
                }
            }
        Snapshot =>
            if (row != nil) {
                status = Ok;
            }
        }
        value: ref Row;
        if (status == Ok && row != nil) {
            value = copy(row, values);
        }
        request.reply <-= Reply(status, value);
    }
}
