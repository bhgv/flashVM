implement Idebridge;
include "uibridge.m";
include "idecore.m";
include "ideinput.m";
include "idebridge.m";


# Split ReplaceAll's payload without delimiter ambiguity or Unicode boundary truncation.
# codec is loaded; payload is borrowed. Returns (1, needle, replacement), or (0, nil, nil).
pair(codec: Uibridge, payload: string): (int, string, string)
{
    bytes := array of byte payload;
    (ok, count) := codec->parseword(bytes, 0);
    if (!ok || count < 0 || count > len bytes - 8) {
        return (0, nil, nil);
    }
    needle := string bytes[8:8 + count];
    replacement := string bytes[8 + count:];
    first := array of byte needle;
    second := array of byte replacement;
    if (len first != count || len second != len bytes - 8 - count) {
        return (0, nil, nil);
    }
    for (i := 0; i < len first; i++) {
        if (first[i] != bytes[8 + i]) {
            return (0, nil, nil);
        }
    }
    for (i = 0; i < len second; i++) {
        if (second[i] != bytes[8 + count + i]) {
            return (0, nil, nil);
        }
    }
    return (1, needle, replacement);
}


# Send a complete document snapshot in bounded, ordered, UTF-8-aligned fragments.
# codec/output remain caller-owned; reply is a value snapshot, never the private document.
# Returns one after all fragments rendezvous, zero on an unexpected encoding failure.
sendreply(codec: Uibridge, output: chan of array of byte, reply: Idecore->Reply, notify: int): int
{
    bytes := array of byte reply.text;
    start := 0;
    for (;;) {
        last := start + ChunkBytes;
        if (last > len bytes) {
            last = len bytes;
        }
        while (last < len bytes && (int bytes[last] & 16rc0) == 16r80) {
            last--;
        }
        metadata := codec->hexword(reply.offset) + codec->hexword(reply.line)
            + codec->hexword(reply.column) + codec->hexword(reply.matches)
            + codec->hexword(reply.selection_end) + codec->hexword(start)
            + codec->hexword(len bytes);
        message := ref Uibridge->Message(codec->Reply, reply.id, reply.status,
            reply.revision, reply.opened, reply.saved, metadata + string bytes[start:last]);
        if (notify) {
            message.kind = codec->View;
            message.id = 0;
        }
        (ok, frame) := codec->encode(message);
        if (!ok) {
            return 0;
        }
        output <-= frame;
        start = last;
        if (start == len bytes) {
            return 1;
        }
    }
}


# Stop the private document owner before a bridge protocol failure returns to its host.
# core/commands/answers are borrowed; revision is the last observed owner revision.
# Returns nothing after receiving the shutdown acknowledgement; no output frame is fabricated.
stop(core: Idecore, commands: chan of Idecore->Request,
     answers: chan of Idecore->Reply, revision: int)
{
    commands <-= Idecore->Request(1, core->Shutdown, revision, 0, 0, "", "", answers);
    <-answers;
}


# Serialize bridge commands through the existing idecore owner and forward raw input separately.
# Parameters/result and blocking ownership are declared in idebridge.m; no Sys or UI module is used.
route(codec: Uibridge, core: Idecore, policy: Ideinput, input: chan of array of byte,
      output: chan of array of byte, events: chan of ref Uibridge->Message): int
{
    if (codec == nil || core == nil || input == nil || output == nil || (events == nil && policy == nil)) {
        return -1;
    }
    commands := core->start();
    answers := chan of Idecore->Reply;
    revision := 0;
    state: ref Ideinput->State;
    if (policy != nil) {
        state = policy->create();
    }
    for (;;) {
        frame := <-input;
        (ok, message) := codec->decode(frame);
        if (!ok) {
            stop(core, commands, answers, revision);
            return -1;
        }
        if (message.kind >= codec->Pointer && message.kind <= codec->Focus) {
            if (policy == nil) {
                events <-= message;
            } else {
                (admitted, view) := policy->apply(core, commands, state, message);
                if (!admitted) {
                    stop(core, commands, answers, revision);
                    return -1;
                }
                revision = view.document.revision;
                if (!sendreply(codec, output, view.document, 1)
                    || !selection(codec, output, state)) {
                    stop(core, commands, answers, revision);
                    return -1;
                }
            }
            continue;
        }
        if (message.kind != codec->Request || message.id <= 0 || message.flags < 0) {
            stop(core, commands, answers, revision);
            return -1;
        }
        text := message.text;
        replacement := "";
        if (message.code == core->ReplaceAll) {
            (valid, needle, value) := pair(codec, message.text);
            if (!valid) {
                stop(core, commands, answers, revision);
                return -1;
            }
            text = needle;
            replacement = value;
        }
        request := Idecore->Request(message.id, message.code, message.flags,
            message.first, message.second, text, replacement, answers);
        commands <-= request;
        reply := <-answers;
        revision = reply.revision;
        if (!sendreply(codec, output, reply, 0)) {
            if (message.code != core->Shutdown || reply.status != core->Ok) {
                stop(core, commands, answers, revision);
            }
            return -1;
        }
        if (policy != nil && (message.code != core->Shutdown || reply.status != core->Ok)) {
            policy->observe(state, reply);
            if (!selection(codec, output, state)) {
                if (message.code != core->Shutdown || reply.status != core->Ok) {
                    stop(core, commands, answers, revision);
                }
                return -1;
            }
        }
        if (message.code == core->Shutdown && reply.status == core->Ok) {
            return 0;
        }
    }
}


# Publish selection decided by Dis; operation 65536 is presentation-only, not an editor command.
# codec/output are borrowed; state is private Dis policy. Returns one after acceptance, zero on error.
selection(codec: Uibridge, output: chan of array of byte, state: ref Ideinput->State): int
{
    message := ref Uibridge->Message(codec->View, 0, 65536, state.revision,
        state.caret, state.anchor, "");
    (ok, frame) := codec->encode(message);
    if (!ok) {
        return 0;
    }
    output <-= frame;
    return 1;
}


# Expose the protocol-only adapter for applications composing a separate Dis input service.
# Parameters/results follow idebridge.m; raw events must be drained by that service.
serve(codec: Uibridge, core: Idecore, input: chan of array of byte,
      output: chan of array of byte, events: chan of ref Uibridge->Message): int
{
    return route(codec, core, nil, input, output, events);
}


# Run editing policy, channel routing and document ownership inside Dis.
# Parameters/results follow idebridge.m; AS only presents the emitted values.
serveapp(codec: Uibridge, core: Idecore, policy: Ideinput,
         input: chan of array of byte, output: chan of array of byte): int
{
    if (policy == nil) {
        return -1;
    }
    return route(codec, core, policy, input, output, nil);
}
