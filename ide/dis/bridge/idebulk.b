implement Idebulk;
include "uibridge.m";
include "unibus.m";
include "idebulk.m";
include "idedesktop.m";


# EN: Send borrowed immutable parts with consecutive byte offsets. Blocks only
# for the output receiver. Encoding failure raises before sending that fragment.
# RU: Передать заимствованные неизменяемые части с последовательными смещениями.
# Ожидает только получателя output. Ошибка кодирования прерывает отправку фрагмента.
fragments(codec: Uibridge, output: chan of array of byte, ticket, slot: int, parts: array of string)
{
    offset := 0;
    for (part := 0; part < len parts; part++) {
        segment := parts[part];
        for (start := 0; start < len segment; start += 1024) {
            end := start + 1024;
            if (end > len segment) {
                end = len segment;
            }
            (valid, wire) := codec->encode(ref Uibridge->Message(codec->View, 0,
                Idedesktop->VectorData, ticket, slot, offset, segment[start:end]));
            if (!valid) {
                raise "fail: bulk geometry encoding";
            }
            output <-= wire;
            offset += end - start;
        }
    }
}


# EN: Announce the exact ASCII extent for slot/revision, then send its parts on
# the same ordered output. Ticket comes from the view owner. Raises on bad encoding.
# RU: Объявить точный размер ASCII для slot/revision, затем передать части в тот же упорядоченный
# output. Билет выдаёт владелец сцены. Ошибка кодирования вызывает исключение.
publish(codec: Uibridge, output: chan of array of byte, ticket, slot, revision: int,
        parts: array of string)
{
    extent := 0;
    for (index := 0; index < len parts; index++) {
        extent += len parts[index];
    }
    (valid, wire) := codec->encode(ref Uibridge->Message(codec->View, 0,
        Idedesktop->VectorBegin, ticket, slot, extent,
        codec->hexword(revision) + codec->hexword(Idedesktop->VectorVersion)));
    if (!valid) {
        raise "fail: paired vector declaration encoding";
    }
    output <-= wire;
    fragments(codec, output, ticket, slot, parts);
}


# Own one rooted native output and one bounded job queue. Immutable job strings
# are shared by reference inside Dis; encoding allocates only <=1024-byte pieces.
# Output backpressure blocks this sender alone. Native routing consumes each
# piece before the scheduler resumes; completed therefore follows last delivery.
# Nil job releases the native borrow, acknowledges once, and terminates.
serve(codec: Uibridge, host: Unibus, slot: int, output: chan of array of byte,
      jobs: chan of ref Job, completed: chan of int)
{
    for (;;) {
        job := <-jobs;
        if (job == nil) {
            if (host->attachbulk(slot, nil) != 0) {
                raise "fail: bulk detach";
            }
            completed <-= slot;
            return;
        }
        if (job.token != 0) {
            # Only this sender assembles the legacy native preview-fence value.
            # The UI owner neither copies the large body nor hashes it.
            drawing := "";
            for (part := 0; part < len job.parts; part++) {
                drawing += job.parts[part];
            }
            host->inkdone(job.token, drawing);
        }
        fragments(codec, output, job.ticket, slot, job.parts);
        completed <-= slot;
        job = nil;
    }
}


# Borrow loaded immutable codec/host modules and a live completion channel.
# Each output is rooted by its spawned owner's stack until acknowledged detach.
# The single waiting job is a bound, not an unbounded snapshot backlog.
start(codec: Uibridge, host: Unibus, slot: int,
      completed: chan of int): chan of ref Job
{
    if (codec == nil || host == nil || slot < 1 || slot > 8 || completed == nil) {
        return nil;
    }
    output := chan of array of byte;
    if (host->attachbulk(slot, output) != 0) {
        return nil;
    }
    jobs := chan[1] of ref Job;
    spawn serve(codec, host, slot, output, jobs, completed);
    return jobs;
}
