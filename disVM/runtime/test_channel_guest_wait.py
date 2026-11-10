"""Exercise guarded channel contention on two production Dis workers."""
from pathlib import Path
import re
import os
import fcntl
import selectors
import subprocess
import sys
import tempfile

from relink_private import relink


SEND_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(other: chan of int)
{
    other <-= 11;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[2] of int;
    spawn send(messages);
    messages <-= 31;
    first := <-messages;
    second := <-messages;
    if (!((first == 11 && second == 31) || (first == 31 && second == 11)))
        raise "channel payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


RECEIVE_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

receive(messages, output: chan of int)
{
    output <-= <-messages;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[3] of int;
    output := chan[1] of int;
    messages <-= 11;
    messages <-= 31;
    spawn receive(messages, output);
    first := <-messages;
    second := <-output;
    if (!((first == 11 && second == 31) || (first == 31 && second == 11)))
        raise "channel receive payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


FAILURE_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

attempt(messages: chan of int, value: int): int
{
    {
        messages <-= value;
        return 1;
    } exception {
        "fixture channel mover failure" => return 0;
    }
}

send(messages, output: chan of int)
{
    output <-= attempt(messages, 11);
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[4] of int;
    output := chan[1] of int;
    spawn send(messages, output);
    sent := attempt(messages, 31);
    other := <-output;
    if (sent + other != 1)
        raise "expected one successful send";
    value := <-messages;
    if ((sent && value != 31) || (other && value != 11))
        raise "failed send published a payload";
    sys->print("PASS guest channel payloads\n");
}
'''


ALT_SEND_GUEST = SEND_GUEST.replace(
    "    messages <-= 31;",
    """    alt {
        messages <-= 31 => {}
    }""",
)


ALT_RECEIVE_GUEST = RECEIVE_GUEST.replace(
    "    first := <-messages;",
    """    first := 0;
    alt {
        first = <-messages => {}
    }""",
)


ALT_FAILURE_GUEST = FAILURE_GUEST.replace(
    "    sent := attempt(messages, 31);",
    """    sent := 0;
    alt {
        messages <-= 31 => sent = 1;
    }""",
)


ALT_READY_GUEST = SEND_GUEST.replace(
    "send(other: chan of int)", "send(other, done: chan of int)"
).replace(
    "    other <-= 11;", "    other <-= 11;\n    done <-= 1;"
).replace(
    "    messages := chan[2] of int;\n    spawn send(messages);",
    "    messages := chan[5] of int;\n    done := chan[1] of int;\n    spawn send(messages, done);",
).replace(
    """    messages <-= 31;
    first := <-messages;
    second := <-messages;""",
    """    available := chan[1] of int;
    alt {
        available <-= 31 => {}
        messages <-= 31 => raise "busy guard selected";
    }
    second := <-available;
    <-done;
    first := <-messages;""",
)


ALT_EMPTY_GUEST = SEND_GUEST.replace(
    """    messages <-= 31;
    first := <-messages;
    second := <-messages;
    if (!((first == 11 && second == 31) || (first == 31 && second == 11)))""",
    """    first := 0;
    alt {
        first = <-messages => {}
    }
    if (first != 11)""",
)


ALT_SHARED_GUEST = ALT_EMPTY_GUEST.replace(
    """    first := 0;
    alt {
        first = <-messages => {}
    }
    if (first != 11)""",
    """    first := 0;
    second := 0;
    alt {
        first = <-messages => {}
        second = <-messages => {}
    }
    if (!((first == 11 && second == 0) || (first == 0 && second == 11)))""",
)


ALT_SHARED_RETRY_GUEST = ALT_SHARED_GUEST.replace(
    "    messages := chan[2] of int;", "    messages := chan[4] of int;"
).replace(
    "    other <-= 11;",
    """    {
        other <-= 99;
        raise "failed send returned normally";
    } exception {
        "fixture channel mover failure" => {}
    }
    other <-= 11;""",
)


ALT_DISTINCT_GUEST = ALT_SHARED_GUEST.replace(
    "    second := 0;", "    second := 0;\n    spare := chan of int;"
).replace(
    "        second = <-messages => {}", "        second = <-spare => raise \"unexpected spare\";"
)


ALT_PEER_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(messages, done: chan of int)
{
    messages <-= 11;
    done <-= 1;
}

sendspare(spare: chan of int)
{
    gate := chan[6] of int;
    if (gate != nil)
        spare <-= 31;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[7] of int;
    done := chan[1] of int;
    spare := chan of int;
    spawn send(messages, done);
    spawn sendspare(spare);
    value := 0;
    alt {
        value = <-messages => raise "reserved channel selected";
        value = <-spare => {}
    }
    <-done;
    if (value != 31 || <-messages != 11)
        raise "channel payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


ALT_ARRAY_GUEST = ALT_EMPTY_GUEST.replace(
    """    first := 0;
    alt {
        first = <-messages => {}
    }
    if (first != 11)""",
    """    channels := array[2] of chan of int;
    channels[1] = messages;
    selected: (int, int);
    alt {
        selected = <-channels => {}
    }
    (index, value) := selected;
    if (index != 1 || value != 11)""",
)


ALT_ARRAY_PEER_GUEST = ALT_PEER_GUEST.replace(
    """    value := 0;
    alt {
        value = <-messages => raise "reserved channel selected";
        value = <-spare => {}
    }
    <-done;
    if (value != 31 || <-messages != 11)""",
    """    channels := array[3] of chan of int;
    channels[1] = messages;
    channels[2] = spare;
    selected: (int, int);
    alt {
        selected = <-channels => {}
    }
    <-done;
    (index, value) := selected;
    if (index != 2 || value != 31 || <-messages != 11)""",
)


STOP_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(messages, done: chan of int)
{
    messages <-= 11;
    done <-= 1;
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[8] of int;
    done := chan[1] of int;
    spawn send(messages, done);
    gate := chan[9] of int;
    if (gate == nil)
        raise "missing gate";
    {
        messages <-= 31;
        raise "cancelled send returned normally";
    } exception {
        "channel wait cancelled" => {}
    }
    <-done;
    if (<-messages != 11)
        raise "channel payload mismatch";
    sys->print("PASS guest channel payloads\n");
}
'''


ALT_STOP_GUEST = STOP_GUEST.replace(
    """    {
        messages <-= 31;
        raise "cancelled send returned normally";""",
    """    spare := chan of int;
    value := 0;
    {
        alt {
            value = <-messages => raise "cancelled receive selected";
            value = <-spare => raise "cancelled spare selected";
        }
        raise "cancelled alt returned normally";""",
)


ISOLATED_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(messages: chan of int)
{
    messages <-= 11;
}

receive(messages: chan of int): int
{
    gate := chan[9] of int;
    if (gate == nil)
        raise "missing gate";
    {
        return <-messages;
    } exception {
        "channel wait cancelled" => return -1;
    }
}

receivechild(messages, output: chan of int)
{
    output <-= receive(messages);
}

init()
{
    sys := load Sys Sys->PATH;
    messages := chan[11] of int;
    output := chan[1] of int;
    spawn send(messages);
    spawn receivechild(messages, output);
    first := receive(messages);
    second := <-output;
    if (!((first == 11 && second == -1) || (first == -1 && second == 11)))
        raise "wrong guest cancellation";
    sys->print("PASS guest channel payloads\n");
}
'''


WORKER_FAILURE_GUEST = STOP_GUEST.replace(
    "init()\n{",
    """failworker()
{
    gate := chan[6] of int;
    if (gate != nil)
        raise "fixture fatal worker error";
}

init()
{""",
).replace(
    "    messages := chan[8] of int;", "    messages := chan[12] of int;"
).replace(
    "    spawn send(messages, done);",
    "    spawn send(messages, done);\n    spawn failworker();",
)


POOL_REFUSAL_GUEST = STOP_GUEST.replace(
    "init()\n{",
    """queued(output: chan of int)
{
    output <-= 7;
}

init()
{""",
).replace(
    "    messages := chan[8] of int;", "    messages := chan[13] of int;"
).replace(
    """    {
        messages <-= 31;""",
    """    output := chan[1] of int;
    spawn queued(output);
    {
        messages <-= 31;""",
).replace(
    '"channel wait cancelled"', '"channel wait needs an executor"'
).replace(
    "    if (<-messages != 11)", "    if (<-messages != 11 || <-output != 7)"
)


ALT_POOL_REFUSAL_GUEST = POOL_REFUSAL_GUEST.replace(
    """    {
        messages <-= 31;
        raise "cancelled send returned normally";""",
    """    spare := chan of int;
    value := 0;
    {
        alt {
            value = <-messages => raise "reserved branch selected";
            value = <-spare => raise "spare branch selected";
        }
        raise "refused alt returned normally";""",
)


THREAD_RETRY_GUEST = POOL_REFUSAL_GUEST.replace("chan[13]", "chan[14]").replace(
    """    <-done;
    if (<-messages != 11 || <-output != 7)""",
    """    messages <-= 31;
    <-done;
    if (<-messages != 11 || <-messages != 31 || <-output != 7)""",
)


ALT_THREAD_RETRY_GUEST = ALT_POOL_REFUSAL_GUEST.replace("chan[13]", "chan[14]").replace(
    """    <-done;
    if (<-messages != 11 || <-output != 7)""",
    """    alt {
        value = <-messages => {}
        value = <-spare => raise "spare branch selected on retry";
    }
    <-done;
    if (value != 11 || <-output != 7)""",
)


IO_REFUSAL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

init()
{
    sys := load Sys Sys->PATH;
    fd := sys->create("io-refusal.bin", Sys->OWRITE, 8r666);
    if (fd == nil || sys->fprint(fd, "a") != 1)
        raise "file setup failed";
    input := sys->open("io-refusal.bin", Sys->OREAD);
    directory := sys->open(".", Sys->OREAD);
    if (input == nil || directory == nil)
        raise "input setup failed";
    bytes := array[1] of byte;
    bytes[0] = byte 98;
    gate := chan[15] of int;
    if (gate == nil)
        raise "missing admission gate";
    if (sys->seek(fd, big 0, Sys->SEEKSTART) != big -1)
        raise "refused seek completed";
    if (sys->write(fd, bytes, 1) != -1)
        raise "refused write completed";
    if (sys->read(input, bytes, 1) != -1 || sys->fprint(fd, "x") != -1)
        raise "refused read or formatted write completed";
    (statstatus, nil) := sys->stat("io-refusal.bin");
    (fdstatus, nil) := sys->fstat(fd);
    (dirstatus, nil) := sys->dirread(directory);
    if (statstatus != -1 || fdstatus != -1 || dirstatus != -1 ||
        sys->open("io-refusal.bin", Sys->OREAD) != nil ||
        sys->create("refused-create.bin", Sys->OWRITE, 8r666) != nil || sys->sleep(1) != -1)
        raise "refused metadata, open or sleep completed";
    if (sys->read(input, bytes, 1) != 1 || bytes[0] != byte 97)
        raise "input cursor or ticket did not recover";
    bytes[0] = byte 98;
    if (sys->write(fd, bytes, 1) != 1)
        raise "output cursor or ticket did not recover";
    sys->print("PASS guest channel payloads\n");
}
'''


SLEEP_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

cancel()
{
    gate := chan[6] of int;
    if (gate == nil)
        raise "missing cancellation gate";
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[16] of int;
    if (mode == nil)
        raise "missing sleep fixture mode";
    spawn cancel();
    if (sys->sleep(60000) != -1)
        raise "sleep cancellation failed";
    started := sys->millisec();
    if (sys->sleep(10) != 0 || sys->millisec() - started < 10)
        raise "subsequent sleep ended early";
    sys->print("PASS guest channel payloads\n");
}
'''


SLEEP_HOST_HANDOFF_GUEST = SLEEP_CANCEL_GUEST.replace("chan[16]", "chan[45]").replace(
    "    spawn cancel();\n", "",
)


SLEEP_WORKER_FAILURE_GUEST = SLEEP_CANCEL_GUEST.replace("chan[16]", "chan[17]").replace(
    '    if (gate == nil)\n        raise "missing cancellation gate";',
    '    if (gate != nil)\n        raise "fixture fatal worker error";',
)


FILE_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

writer(fd: ref Sys->FD, done: chan of int)
{
    sys := load Sys Sys->PATH;
    done <-= sys->fprint(fd, "a");
}

cancel()
{
    gate := chan[6] of int;
    if (gate == nil)
        raise "missing cancellation gate";
}

init()
{
    sys := load Sys Sys->PATH;
    fd := sys->create("file-cancel.bin", Sys->OWRITE, 8r666);
    mode := chan[18] of int;
    done := chan[1] of int;
    if (fd == nil || mode == nil)
        raise "file setup failed";
    spawn writer(fd, done);
    spawn cancel();
    gate := chan[19] of int;
    if (gate == nil || sys->fprint(fd, "x") != -1)
        raise "queued write was not cancelled";
    if (<-done != 1 || sys->fprint(fd, "b") != 1)
        raise "shared cursor did not recover";
    sys->print("PASS guest channel payloads\n");
}
'''


STDIN_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

cancel()
{
    gate := chan[6] of int;
    if (gate == nil)
        raise "missing cancellation gate";
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[20] of int;
    if (mode == nil)
        raise "missing stdin fixture mode";
    input := sys->fildes(0);
    bytes := array[1] of byte;
    spawn cancel();
    if (sys->read(input, bytes, 1) != -1)
        raise "blocked stdin read was not cancelled";
    sys->print("READY after cancelled stdin\n");
    if (sys->read(input, bytes, 1) != 1 || bytes[0] != byte 90)
        raise "stdin did not recover after cancellation";
    sys->print("PASS guest channel payloads\n");
}
'''


STDIN_WORKER_FAILURE_GUEST = STDIN_CANCEL_GUEST.replace("chan[20]", "chan[21]").replace(
    '    if (gate == nil)\n        raise "missing cancellation gate";',
    '    if (gate != nil)\n        raise "fixture fatal worker error";',
)


STDIN_PARTIAL_GUEST = STDIN_CANCEL_GUEST.replace("chan[20]", "chan[22]").replace(
    """    bytes := array[1] of byte;
    spawn cancel();
    if (sys->read(input, bytes, 1) != -1)
        raise "blocked stdin read was not cancelled";""",
    """    bytes := array[2] of byte;
    bytes[1] = byte 89;
    spawn cancel();
    if (sys->readn(input, bytes, 2) != 1 || bytes[0] != byte 65 || bytes[1] != byte 89)
        raise "cancelled readn lost its completed fragment or touched the tail";""",
)


def run_stdin_fixture(command, work, failure, partial):
    """Keep a real pipe empty until the cancelled guest reports it can retry."""
    with subprocess.Popen(command, cwd=work, stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE) as process:
        output = bytearray()
        try:
            if partial:
                process.stdin.write(b"A")
                process.stdin.flush()
            if failure:
                process.wait(timeout=15)
                after, errors = process.communicate()
            else:
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stdout, selectors.EVENT_READ)
                    while b"READY after cancelled stdin\n" not in output:
                        if not selector.select(timeout=15):
                            raise RuntimeError("stdin cancellation did not wake the guest")
                        fragment = os.read(process.stdout.fileno(), 4096)
                        if not fragment:
                            raise RuntimeError("stdin guest exited before retry")
                        output.extend(fragment)
                after, errors = process.communicate(input=b"Z", timeout=15)
        except BaseException as failure:
            process.kill()
            after, errors = process.communicate()
            raise RuntimeError(f"{command[-1]} {command[1]}\n{(output + after).decode()}\n{errors.decode()}") from failure
        return subprocess.CompletedProcess(command, process.returncode,
                                           (output + after).decode(), errors.decode())


STDOUT_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

cancel()
{
    gate := chan[6] of int;
    if (gate == nil)
        raise "missing cancellation gate";
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[23] of int;
    if (mode == nil)
        raise "missing stdout fixture mode";
    output := sys->fildes(1);
    spawn cancel();
    if (sys->fprint(output, "x") != -1)
        raise "blocked output was not cancelled";
    sys->fprint(sys->fildes(2), "READY after cancelled stdout\n");
    if (sys->fprint(output, "b") != 1)
        raise "stdout did not recover";
    sys->print("PASS guest channel payloads\n");
}
'''


STDOUT_PARTIAL_GUEST = STDOUT_CANCEL_GUEST.replace("chan[23]", "chan[24]").replace(
    '    if (sys->fprint(output, "x") != -1)',
    """    bytes := array[8192] of byte;
    for (index := 0; index < len bytes; index++)
        bytes[index] = byte 97;
    if (sys->write(output, bytes, len bytes) != 4096)""",
)


STDOUT_WORKER_FAILURE_GUEST = STDOUT_CANCEL_GUEST.replace("chan[23]", "chan[25]").replace(
    '    if (gate == nil)\n        raise "missing cancellation gate";',
    '    if (gate != nil)\n        raise "fixture fatal worker error";',
).split('    sys->fprint(sys->fildes(2), "READY after cancelled stdout', 1)[0] + "}\n"


def run_stdout_fixture(command, work, failure, partial):
    """Prefill a real pipe and leave it full until the guest reports cancellation."""
    read_fd, write_fd = os.pipe()
    try:
        page_size = os.sysconf("SC_PAGE_SIZE")
        capacity = fcntl.fcntl(write_fd, fcntl.F_SETPIPE_SZ, max(8192, page_size))
        prefix = b"p" * (capacity - (page_size if partial else 0))
        if os.write(write_fd, prefix) != len(prefix):
            raise RuntimeError("could not fill fixture pipe")
        with subprocess.Popen(command, cwd=work, stdout=write_fd, stderr=subprocess.PIPE) as process:
            os.close(write_fd)
            write_fd = None
            errors = bytearray()
            try:
                marker = (b"PASS cancelled stdout wait\n" if failure
                          else b"READY after cancelled stdout\n")
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stderr, selectors.EVENT_READ)
                    while marker not in errors:
                        if not selector.select(timeout=15):
                            raise RuntimeError("stdout cancellation did not wake the guest")
                        fragment = os.read(process.stderr.fileno(), 4096)
                        if not fragment:
                            raise RuntimeError("stdout guest exited before cancellation")
                        errors.extend(fragment)
                expected = prefix + (b"a" * 4096 if partial else b"")
                received = bytearray()
                while len(received) < len(expected):
                    fragment = os.read(read_fd, len(expected) - len(received))
                    if not fragment:
                        raise RuntimeError("pipe closed before committed prefix")
                    received.extend(fragment)
                if received != expected:
                    raise RuntimeError("pipe contains bytes outside the committed prefix")
                output = bytearray()
                with selectors.DefaultSelector() as selector:
                    selector.register(read_fd, selectors.EVENT_READ)
                    while True:
                        if not selector.select(timeout=15):
                            raise RuntimeError("stdout retry did not complete")
                        fragment = os.read(read_fd, capacity)
                        if not fragment:
                            break
                        output.extend(fragment)
                _, after = process.communicate(timeout=15)
                if failure and not output.startswith(b"MANAGEDOWNERS "):
                    raise RuntimeError(f"failed guest changed the full pipe: {output!r}")
                if not failure and not output.startswith(b"bPASS guest channel payloads\n"):
                    raise RuntimeError(f"retry wrote unexpected bytes {output!r}")
            except BaseException as problem:
                process.kill()
                _, after = process.communicate()
                raise RuntimeError(f"{command[-1]} {command[1]}\n{(errors + after).decode()}") from problem
            return subprocess.CompletedProcess(command, process.returncode,
                                               output.decode(), (errors + after).decode())
    finally:
        os.close(read_fd)
        if write_fd is not None:
            os.close(write_fd)


STDOUT_ALIAS_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

writer(done: chan of int)
{
    sys := load Sys Sys->PATH;
    bytes := array[8192] of byte;
    for (index := 0; index < len bytes; index++)
        bytes[index] = byte 97;
    done <-= sys->write(sys->fildes(1), bytes, len bytes);
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[26] of int;
    done := chan[1] of int;
    if (mode == nil)
        raise "missing alias fixture mode";
    spawn writer(done);
    gate := chan[19] of int;
    if (gate == nil || sys->fprint(sys->fildes(2), "b") != 1 || <-done != 8192)
        raise "aliased output failed";
    sys->print("PASS guest channel payloads\n");
}
'''


STDOUT_ALIAS_CANCEL_GUEST = STDOUT_ALIAS_GUEST.replace("chan[26]", "chan[27]").replace(
    "init()\n{",
    """cancel()
{
    gate := chan[6] of int;
    if (gate == nil)
        raise "missing cancellation gate";
}

init()
{""",
).replace(
    "    spawn writer(done);", "    spawn writer(done);\n    spawn cancel();",
).replace(
    '    if (gate == nil || sys->fprint(sys->fildes(2), "b") != 1 || <-done != 8192)',
    '    if (gate == nil || sys->fprint(sys->fildes(2), "x") != -1 ||\n'
    '        <-done != 8192 || sys->fprint(sys->fildes(2), "b") != 1)',
)


BROKEN_PIPE_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

peer(done: chan of int)
{
    done <-= 7;
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[28] of int;
    output := sys->fildes(1);
    done := chan[1] of int;
    if (mode == nil)
        raise "missing broken pipe fixture mode";
    spawn peer(done);
    if (sys->fprint(output, "x") != -1 || sys->fprint(output, "y") != -1 || <-done != 7)
        raise "broken pipe did not preserve guest scheduling";
    sys->fprint(sys->fildes(2), "PASS guest channel payloads\n");
}
'''


BROKEN_PIPE_PARTIAL_GUEST = BROKEN_PIPE_GUEST.replace("chan[28]", "chan[29]").replace(
    '    if (sys->fprint(output, "x") != -1 || sys->fprint(output, "y") != -1 || <-done != 7)',
    """    bytes := array[8192] of byte;
    for (index := 0; index < len bytes; index++)
        bytes[index] = byte 97;
    if (sys->write(output, bytes, len bytes) != 4096 ||
        sys->fprint(output, "y") != -1 || <-done != 7)""",
)


def run_broken_pipe_fixture(command, work, partial):
    """Close the external reader after poll has admitted the next splice."""
    read_fd, write_fd = os.pipe()
    try:
        with subprocess.Popen(command, cwd=work, stdin=subprocess.PIPE, stdout=write_fd,
                              stderr=subprocess.PIPE) as process:
            os.close(write_fd)
            write_fd = None
            errors = bytearray()
            try:
                with selectors.DefaultSelector() as selector:
                    selector.register(process.stderr, selectors.EVENT_READ)
                    while b"READY close stdout reader\n" not in errors:
                        if not selector.select(timeout=15):
                            raise RuntimeError("guest did not reach the broken pipe splice")
                        fragment = os.read(process.stderr.fileno(), 4096)
                        if not fragment:
                            raise RuntimeError("guest exited before the reader closed")
                        errors.extend(fragment)
                if partial and os.read(read_fd, 4096) != b"a" * 4096:
                    raise RuntimeError("first output fragment was not committed intact")
                os.close(read_fd)
                read_fd = None
                _, after = process.communicate(input=b"x", timeout=15)
            except BaseException:
                process.kill()
                process.communicate()
                raise
            return subprocess.CompletedProcess(command, process.returncode, "", (errors + after).decode())
    finally:
        if read_fd is not None:
            os.close(read_fd)
        if write_fd is not None:
            os.close(write_fd)


STDOUT_ALIAS_PROTECTED_GUEST = STDOUT_ALIAS_CANCEL_GUEST.replace("chan[27]", "chan[30]").replace(
    "    spawn cancel();\n", "",
)


DIRECTORY_PUBLICATION_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

reader(fd: ref Sys->FD, done: chan of int)
{
    sys := load Sys Sys->PATH;
    {
        sys->dirread(fd);
        raise "directory publication failure was lost";
    } exception {
        "fixture directory publication failure" => done <-= 1;
    }
}

init()
{
    sys := load Sys Sys->PATH;
    fd := sys->open("directory-publication", Sys->OREAD);
    mode := chan[31] of int;
    done := chan[1] of int;
    if (fd == nil || mode == nil)
        raise "directory setup failed";
    spawn reader(fd, done);
    gate := chan[19] of int;
    if (gate == nil)
        raise "missing directory gate";
    (status, entries) := sys->dirread(fd);
    if (status != -1 || entries != nil || <-done != 1)
        raise "competing read overtook failed publication";
    fresh := sys->open("directory-publication", Sys->OREAD);
    (count, records) := sys->dirread(fresh);
    if (count != 1 || records[0].name != "entry")
        raise "independent directory cursor did not recover";
    sys->print("PASS guest channel payloads\n");
}
'''


CLOSE_PARKED_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

waiter(input, report: chan of int)
{
    {
        WAIT_OPERATION;
        raise "closed waiter returned normally";
    } exception {
        "channel hangup" => report <-= 1;
    }
}

init()
{
    sys := load Sys Sys->PATH;
    input := chan of int;
    report := chan[1] of int;
    spawn waiter(input, report);
    for (attempt := 0; attempt < 1000; attempt++)
    {
        alt {
            <-report =>
                sys->print("PASS dedicated channel close\n");
                return;
            * => sys->sleep(1);
        }
    }
    raise "closed waiter did not resume";
}
'''


PARKED_SEND_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

waiter(first, second, done: chan of int)
{
    {
        first <-= 99;
        raise "cancelled guest continued normally";
    } exception {
        "channel wait cancelled" => done <-= 1;
    }
}

send(first, second: chan of int)
{
    first <-= 11;
    second <-= 31;
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[32] of int;
    first := chan of int;
    second := chan of int;
    done := chan[1] of int;
    if (mode == nil)
        raise "missing cancellation mode";
    spawn waiter(first, second, done);
    gate := chan[33] of int;
    if (gate == nil || <-done != 1)
        raise "cancelled waiter did not handle the exception";
    alt {
        <-first => raise "stale sender registration";
        <-second => raise "stale second registration";
        * => {}
    }
    spawn send(first, second);
    if (<-first != 11 || <-second != 31)
        raise "channel reuse failed";
    sys->print("PASS guest channel payloads\n");
}
'''


PARKED_RECEIVE_GUEST = PARKED_SEND_GUEST.replace("        first <-= 99;", "        <-first;")


PARKED_ALT_GUEST = PARKED_SEND_GUEST.replace(
    "        first <-= 99;",
    """        alt {
            <-first => {}
            <-second => {}
        }""",
)


PARKED_ARRAY_ALT_GUEST = PARKED_SEND_GUEST.replace(
    "    {\n        first <-= 99;",
    """    channels := array[3] of chan of int;
    channels[0] = first;
    channels[2] = second;
    selected: (int, int);
    {
        alt {
            selected = <-channels => {}
        }""",
)


PARKED_ISOLATION_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

receiver(input, output: chan of int)
{
    result := 0;
    {
        result = <-input;
    } exception {
        "channel wait cancelled" => result = -1;
    }
    output <-= result;
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[34] of int;
    input := chan of int;
    output := chan[1] of int;
    if (mode == nil)
        raise "missing isolation fixture";
    spawn receiver(input, output);
    spawn receiver(input, output);
    gate := chan[33] of int;
    if (gate == nil)
        raise "missing cancellation coordinator";
    input <-= 11;
    first := <-output;
    second := <-output;
    if (!((first == -1 && second == 11) || (first == 11 && second == -1)))
        raise "cancelled the wrong number of receivers";
    sys->print("PASS guest channel payloads\n");
}
'''


HOST_CANCEL_GUEST = r'''implement ChannelWait;
include "sys.m";
ChannelWait: module { init: fn(); };

send(input: chan of int)
{
    input <-= 11;
}

init()
{
    sys := load Sys Sys->PATH;
    mode := chan[35] of int;
    input := chan of int;
    if (mode == nil)
        raise "missing host cancellation mode";
    {
        <-input;
        raise "host cancellation did not arrive";
    } exception {
        "channel wait cancelled" => {}
    }
    spawn send(input);
    if (<-input != 11)
        raise "channel reuse failed after host cancellation";
    sys->print("PASS guest channel payloads\n");
}
'''


PROCESSCTL_GUEST = r'''implement ChannelWait;
include "sys.m";
include "processctl.m";
ChannelWait: module { init: fn(); };

waiter(ids, result: chan of int)
{
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    input := chan of int;
    {
        <-input;
        raise "cancelled child resumed normally";
    } exception {
        "channel wait cancelled" => result <-= 11;
    }
}

sleeper(ids, result: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    if (sys->sleep(60000) != -1)
        raise "child sleep was not cancelled";
    result <-= 31;
}

sibling(target: int, result: chan of int)
{
    control := load Processctl Processctl->PATH;
    result <-= control->cancelwait(target);
}

cancel(pid: int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    for (attempt := 0; attempt < 1000; attempt++)
    {
        case control->cancelwait(pid) {
        Processctl->Cancelled => return;
        Processctl->NotWaiting => sys->sleep(1);
        * => raise "parent was denied cancellation";
        }
    }
    raise "child did not publish its wait";
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    ids := chan of int;
    result := chan[1] of int;
    if (control == nil || control->pid() <= 0 ||
        control->cancelwait(control->pid()) != Processctl->Denied ||
        control->cancelwait(0) != Processctl->Denied ||
        control->cancelwait(999999) != Processctl->Denied)
        raise "process control scope failed";
    spawn waiter(ids, result);
    parked := <-ids;
    spawn sibling(parked, result);
    if (<-result != Processctl->Denied)
        raise "sibling could cancel a foreign child";
    cancel(parked);
    if (<-result != 11)
        raise "parked child cancellation failed";
    spawn sleeper(ids, result);
    sleeping := <-ids;
    if (parked == sleeping)
        raise "runtime recycled a process identity";
    cancel(sleeping);
    if (<-result != 31)
        raise "native child cancellation failed";
    sys->print("PASS guest process control\n");
}
'''


PROCESSCTL_INLINE_GUEST = PROCESSCTL_GUEST.split("    spawn sleeper(ids, result);", 1)[0] + (
    '    sys->print("PASS guest process control\\n");\n}\n'
)


PROCESS_KILL_GUEST = r'''implement ChannelWait;
include "sys.m";
include "processctl.m";
ChannelWait: module { init: fn(); };

child(ids, escaped: chan of int)
{
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    {
        input := chan of int;
        <-input;
        escaped <-= 1;
    } exception {
        * => escaped <-= 2;
    }
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    mode := chan[32] of int;
    ids := chan of int;
    escaped := chan[1] of int;
    if (mode == nil || control->kill(control->pid()) != Processctl->Denied ||
        control->kill(999999) != Processctl->Denied)
        raise "kill authority failed";
    spawn child(ids, escaped);
    target := <-ids;
    gate := chan[37] of int;
    if (gate == nil || control->kill(target) != Processctl->Requested)
        raise "kill request failed";
    for (attempt := 0; attempt < 1000; attempt++)
    {
        if (control->cancelwait(target) == Processctl->Denied)
        {
            alt {
                <-escaped => raise "terminated guest executed or caught an exception";
                * => {}
            }
            sys->print("PASS guest process termination\n");
            return;
        }
        sys->sleep(1);
    }
    raise "terminated child did not retire";
}
'''


PROCESS_KILL_NATIVE_GUEST = PROCESS_KILL_GUEST.replace(
    "    control := load Processctl Processctl->PATH;\n    ids <-=",
    "    sys := load Sys Sys->PATH;\n    control := load Processctl Processctl->PATH;\n    ids <-=",
).replace(
    "        input := chan of int;\n        <-input;",
    "        sys->sleep(60000);",
).replace("chan[32]", "chan[38]").replace("chan[37]", "chan[6]")


PROCESS_KILL_READY_GUEST = PROCESS_KILL_GUEST.replace(
    "        input := chan of int;\n        <-input;",
    "        while (1) {}",
).replace("chan[32]", "chan[39]").replace("chan[37]", "chan[1]")


PROCESS_KILL_TABLE_GUEST = r'''implement ChannelWait;
include "sys.m";
include "processctl.m";
include "bigtable.m";
ChannelWait: module { init: fn(); };

worker(tab: ref Bigtable->Handle, ids, results: chan of int, value: int)
{
    tables := load Bigtable Bigtable->PATH;
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    {
        tables->acquire(tab);
        tables->requireok(tables->setindex(tab, big 0, big value));
        tables->release(tab);
        results <-= value;
    } exception message {
        * => raise message;
    }
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    tables := load Bigtable Bigtable->PATH;
    fixture := chan[39] of int;
    if (fixture == nil)
        raise "missing table fixture";
    tab := tables->create();
    ids := chan of int;
    results := chan[1] of int;
    tables->acquire(tab);
    spawn worker(tab, ids, results, 11);
    victim := <-ids;
    spawn worker(tab, ids, results, 31);
    survivor := <-ids;
    for (attempt := 0; attempt < 1000; attempt++)
    {
        (token, depth, waiting) := tables->guardstate(tab);
        if (token != big 0 && depth == 1 && waiting == 2)
            break;
        sys->sleep(1);
    }
    (token, depth, waiting) := tables->guardstate(tab);
    if (victim == survivor || token == big 0 || depth != 1 || waiting != 2 ||
        control->kill(victim) != Processctl->Requested)
        raise "could not kill a table-blocked child";
    (token, depth, waiting) = tables->guardstate(tab);
    if (token == big 0 || depth != 1 || waiting != 1)
        raise "kill disturbed the retained lease or surviving waiter";
    tables->release(tab);
    if (<-results != 31)
        raise "wrong table waiter resumed";
    (status, value) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || value != big 31)
        raise "surviving table write was lost";
    for (attempt = 0; attempt < 1000; attempt++)
    {
        if (control->cancelwait(victim) == Processctl->Denied)
        {
            alt {
                <-results => raise "terminated table guest ran after kill";
                * => {}
            }
            sys->print("PASS guest process termination\n");
            return;
        }
        sys->sleep(1);
    }
    raise "table-blocked child did not retire";
}
'''


TABLE_FIFO_GUEST = r'''implement ChannelWait;
include "sys.m";
include "bigtable.m";
ChannelWait: module { init: fn(); };

waitqueued(tables: Bigtable, tab: ref Bigtable->Handle, count: int)
{
    sys := load Sys Sys->PATH;
    for (attempt := 0; attempt < 1000; attempt++)
    {
        (nil, nil, waiting) := tables->guardstate(tab);
        if (waiting == count)
            return;
        sys->sleep(1);
    }
    raise "table waiter did not queue";
}

writer(tables: Bigtable, tab: ref Bigtable->Handle, expected, next: big, done: chan of int)
{
    tables->acquire(tab);
    (status, previous) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || previous != expected)
        raise "table waiter overtook an older request";
    tables->requireok(tables->setindex(tab, big 0, next));
    tables->release(tab);
    done <-= 1;
}

init()
{
    sys := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    tab := tables->create();
    done := chan[1] of int;
    tables->acquire(tab);
    tables->requireok(tables->setindex(tab, big 0, big 7));
    spawn writer(tables, tab, big 7, big 11, done);
    waitqueued(tables, tab, 1);
    spawn writer(tables, tab, big 11, big 31, done);
    waitqueued(tables, tab, 2);
    tables->release(tab);
    tables->acquire(tab);
    (status, value) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || value != big 31)
        raise "new acquisition bypassed the queued table writers";
    tables->release(tab);
    if (<-done != 1 || <-done != 1)
        raise "queued table writers failed";
    sys->print("PASS cooperative table FIFO\n");
}
'''


TABLE_FIFO_KILL_HEAD_GUEST = PROCESS_KILL_TABLE_GUEST.replace(
    """    if (victim == survivor || token == big 0 || depth != 1 || waiting != 2 ||
        control->kill(victim) != Processctl->Requested)
        raise "could not kill a table-blocked child";
    (token, depth, waiting) = tables->guardstate(tab);
    if (token == big 0 || depth != 1 || waiting != 1)
        raise "kill disturbed the retained lease or surviving waiter";
    tables->release(tab);""",
    """    if (victim == survivor || token == big 0 || depth != 1 || waiting != 2)
        raise "table contenders did not queue";
    tables->release(tab);
    if (control->kill(victim) != Processctl->Requested)
        raise "could not kill the granted queue head";""",
)


TABLE_FIFO_MOVE_GUEST = TABLE_FIFO_GUEST.split("writer(", 1)[0] + r'''hold(tables: Bigtable, tab: ref Bigtable->Handle, ready, release: chan of int)
{
    tables->acquire(tab);
    ready <-= 1;
    <-release;
    tables->release(tab);
}

merge(tables: Bigtable, dest, source: ref Bigtable->Handle, done: chan of int)
{
    tables->requireok(tables->splice(dest, big 0, big 1, source));
    done <-= 31;
}

update(tables: Bigtable, tab: ref Bigtable->Handle, done: chan of int)
{
    tables->acquire(tab);
    tables->requireok(tables->setindex(tab, big 0, big 11));
    tables->release(tab);
    done <-= 11;
}

init()
{
    sys := load Sys Sys->PATH;
    tables := load Bigtable Bigtable->PATH;
    dest := tables->create();
    source := tables->create();
    ready := chan[1] of int;
    release := chan[1] of int;
    done := chan[1] of int;
    tables->requireok(tables->setindex(dest, big 0, big 7));
    tables->requireok(tables->setindex(source, big 0, big 31));
    tables->acquire(dest);
    spawn hold(tables, source, ready, release);
    <-ready;
    spawn merge(tables, dest, source, done);
    waitqueued(tables, dest, 1);
    spawn update(tables, dest, done);
    waitqueued(tables, dest, 2);
    tables->release(dest);
    waitqueued(tables, source, 1);
    if (<-done != 11)
        raise "moving a grant stranded the previous queue";
    release <-= 1;
    if (<-done != 31)
        raise "two-table operation did not resume";
    (status, value) := tables->getindex(dest, big 0);
    if (status != Bigtable->Ok || value != big 31)
        raise "two-table result was lost";
    sys->print("PASS cooperative table FIFO\n");
}
'''


TABLE_CANCEL_GUEST = TABLE_FIFO_GUEST.split("writer(", 1)[0].replace(
    'include "bigtable.m";', 'include "bigtable.m";\ninclude "processctl.m";'
) + r'''victim(tables: Bigtable, tab: ref Bigtable->Handle, ids, reports, again: chan of int)
{
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    {
        tables->acquire(tab);
        raise "cancelled MCALL unexpectedly acquired the table";
    } exception {
        "table lease wait cancelled" => reports <-= -1;
    }
    <-again;
    tables->acquire(tab);
    tables->requireok(tables->setindex(tab, big 0, big 51));
    tables->release(tab);
    reports <-= 51;
}

survivor(tables: Bigtable, tab: ref Bigtable->Handle, done: chan of int)
{
    tables->acquire(tab);
    tables->requireok(tables->setindex(tab, big 0, big 31));
    tables->release(tab);
    done <-= 31;
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    tables := load Bigtable Bigtable->PATH;
    tab := tables->create();
    ids := chan of int;
    reports := chan[1] of int;
    again := chan[1] of int;
    done := chan[1] of int;
    tables->acquire(tab);
    spawn victim(tables, tab, ids, reports, again);
    pid := <-ids;
    waitqueued(tables, tab, 1);
    spawn survivor(tables, tab, done);
    waitqueued(tables, tab, 2);
    if (control->cancelwait(pid) != Processctl->Cancelled ||
        control->cancelwait(pid) == Processctl->Cancelled)
        raise "table cancellation did not remove exactly one wait";
    tables->release(tab);
    if (<-reports != -1 || <-done != 31)
        raise "table cancellation or surviving waiter failed";
    again <-= 1;
    if (<-reports != 51)
        raise "cancelled guest did not retry";
    (status, value) := tables->getindex(tab, big 0);
    if (status != Bigtable->Ok || value != big 51)
        raise "retry did not publish its value";
    sys->print("PASS cancelled table retry\n");
}
'''


TABLE_CANCEL_GRANTED_GUEST = TABLE_CANCEL_GUEST.replace(
    "    if (control->cancelwait(pid)", "    tables->release(tab);\n    if (control->cancelwait(pid)"
).replace(
    "    tables->release(tab);\n    if (<-reports", "    if (<-reports"
)


def killwait_guest(guest):
    """Retire the same child without a timer or a separate wait-state query."""
    prefix = guest.split("    if (gate == nil || control->kill(target)", 1)[0]
    return prefix + r'''    if (gate == nil || control->killwait(target) != Processctl->Retired ||
        control->cancelwait(target) != Processctl->Denied ||
        control->killwait(target) != Processctl->Denied)
        raise "killwait returned before retirement or replayed completion";
    alt {
        <-escaped => raise "terminated guest executed after killwait";
        * => {}
    }
    sys->print("PASS guest child retirement\n");
}
'''


PROCESS_KILLWAIT_TABLE_GUEST = PROCESS_KILL_TABLE_GUEST.replace(
    "        control->kill(victim) != Processctl->Requested)",
    "        control->killwait(victim) != Processctl->Retired ||\n"
    "        control->cancelwait(victim) != Processctl->Denied)",
).replace("PASS guest process termination", "PASS guest child retirement")


KILLWAIT_CHILDREN = r'''implement ChannelWait;
include "sys.m";
include "processctl.m";
ChannelWait: module { init: fn(); };

child(fd: ref Sys->FD, ids, reports: chan of int)
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    ids <-= control->pid();
    sys->fprint(fd, "a");
    reports <-= 99;
}

parent(fd: ref Sys->FD, ids, reports: chan of int)
{
    control := load Processctl Processctl->PATH;
    nested := chan of int;
    ids <-= control->pid();
    spawn child(fd, nested, reports);
    target := <-nested;
    gate := chan[19] of int;
    if (gate == nil)
        raise "child I/O did not begin";
    {
        control->killwait(target);
        raise "first killwait was not interrupted";
    } exception {
        "child exit wait cancelled" => reports <-= 1;
    }
    if (control->killwait(target) != Processctl->Retired ||
        control->cancelwait(target) != Processctl->Denied)
        raise "second killwait did not observe the original child exit";
    reports <-= 2;
}

'''

KILLWAIT_CANCEL_GUEST = KILLWAIT_CHILDREN + r'''
init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    mode := chan[40] of int;
    ids := chan of int;
    reports := chan[1] of int;
    fd := sys->create("killwait-cancel.bin", Sys->OWRITE, 8r666);
    if (mode == nil || fd == nil)
        raise "missing exit cancellation setup";
    spawn parent(fd, ids, reports);
    target := <-ids;
    first := chan[41] of int;
    if (first == nil || control->cancelwait(target) != Processctl->Cancelled ||
        control->cancelwait(target) == Processctl->Cancelled || <-reports != 1)
        raise "could not cancel the parent's first wait";
    second := chan[42] of int;
    if (second == nil || <-reports != 2)
        raise "parent did not resume after child retirement";
    alt {
        <-reports => raise "child executed after termination request";
        * => {}
    }
    sys->print("PASS guest child retirement\n");
}
'''

KILLWAIT_HOST_GUEST = KILLWAIT_CANCEL_GUEST.replace(
    "    control := load Processctl Processctl->PATH;\n    mode := chan[40] of int;",
    "    mode := chan[43] of int;",
).replace(
    "killwait-cancel.bin", "killwait-host.bin",
).replace(
    "    target := <-ids;\n"
    "    first := chan[41] of int;\n"
    "    if (first == nil || control->cancelwait(target) != Processctl->Cancelled ||\n"
    "        control->cancelwait(target) == Processctl->Cancelled || <-reports != 1)",
    "    if (<-ids <= 0 || <-reports != 1)",
)


ALT_DEFAULT_GUEST = ALT_READY_GUEST.replace(
    '        available <-= 31 => {}\n        messages <-= 31 => raise "busy guard selected";',
    '        messages <-= 31 => raise "busy guard selected";\n        * => available <-= 31;',
)


KILLWAIT_NESTED_GUEST = KILLWAIT_CHILDREN + r'''release_write(done: chan of int)
{
    second := chan[42] of int;
    if (second == nil)
        raise "missing second killwait";
    done <-= 1;
}

init()
{
    sys := load Sys Sys->PATH;
    control := load Processctl Processctl->PATH;
    mode := chan[40] of int;
    ids := chan of int;
    reports := chan[1] of int;
    done := chan of int;
    fd := sys->create("killwait-nested.bin", Sys->OWRITE, 8r666);
    if (mode == nil || fd == nil)
        raise "missing nested exit setup";
    spawn parent(fd, ids, reports);
    target := <-ids;
    first := chan[41] of int;
    if (first == nil)
        raise "missing first killwait";
    spawn release_write(done);
    if (control->killwait(target) != Processctl->Retired || <-done != 1)
        raise "parked parent was not retired";
    alt {
        <-reports => raise "terminated guest resumed its own Limbo code";
        * => {}
    }
    sys->print("PASS guest child retirement\n");
}
'''



def main():
    """Use the actual allocator and wait path with an injected slow mover."""
    runtime, compiler, include = (Path(arg).resolve() for arg in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix="channel-guest-wait-") as directory:
        work = Path(directory)
        if "--controller-wake-only" in sys.argv[4:]:
            binary = relink(runtime, work, Path(__file__).with_name("test_controller_wake.c"),
                            0, ["dvm_runqueue_claim", "dvm_managed_workers_wake_guest",
                                "dvm_managed_workers_park"])
            source, image = work / "controller-wake.b", work / "controller-wake.dis"
            source.write_text(TABLE_FIFO_MOVE_GUEST)
            subprocess.run([compiler, "-I", include,
                            "-I", Path(__file__).resolve().parents[1] / "module",
                            "-o", image, source], check=True)
            for mode in ("--no-jit", "--jit"):
                result = subprocess.run(
                    [binary, mode, "--native-spawn", "--guarded-copies", image],
                    cwd=work, capture_output=True, text=True, timeout=5,
                )
                if (result.returncode or "PASS controller wake before park" not in result.stdout
                        or "PASS cooperative table FIFO" not in result.stdout):
                    raise RuntimeError(f"{mode} controller wake\n" + result.stdout + result.stderr)
            return
        if "--table-fifo-only" in sys.argv[4:]:
            for name, guest in (("table-fifo", TABLE_FIFO_GUEST),
                                ("table-fifo-move", TABLE_FIFO_MOVE_GUEST)):
                source, image = work / f"{name}.b", work / f"{name}.dis"
                source.write_text(guest)
                subprocess.run([compiler, "-I", include,
                                "-I", Path(__file__).resolve().parents[1] / "module",
                                "-o", image, source], check=True)
                for mode in ("--no-jit", "--jit"):
                    for attempt in range(8):
                        result = subprocess.run(
                            [runtime, mode, "--native-spawn", "--guarded-copies", image],
                            cwd=work, capture_output=True, text=True, timeout=5,
                        )
                        if result.returncode or "PASS cooperative table FIFO" not in result.stdout:
                            raise RuntimeError(f"{mode} {name} attempt {attempt + 1} "
                                               f"exited {result.returncode}\n" + result.stdout + result.stderr)
            return
        table_cancel = "--table-cancel-only" in sys.argv[4:]
        binary = runtime
        table_kill_head = "--table-kill-head-only" in sys.argv[4:]
        table_cancel_head = "--table-cancel-head-only" in sys.argv[4:]
        if table_kill_head or table_cancel_head:
            binary = relink(runtime, work, Path(__file__).with_name("test_table_kill_head.c"),
                            0, ["dvm_bigtable_call_ready", "dvm_runtime_kill_child",
                                "dvm_runtime_cancel_child_wait", "dvm_runqueue_claim"])
        elif not table_cancel:
            binary = relink(runtime, work, Path(__file__).with_suffix(".c").resolve(), 0,
                            ["dvm_runtime_channel_new", "dvm_runtime_channel_wait_guest", "xecalt",
                             "dvm_runtime_channel_wait_change", "fvm_port_thread_start",
                             "dvm_managed_workers_native_wait", "dvm_runtime_guest_sleep",
                             "fvm_port_sync_wait_ms", "fvm_port_file_write",
                             "dvm_file_gate_try_release", "dvm_file_gate_enter",
                             "dvm_runtime_read_stdin", "dvm_runtime_write_stdio", "splice",
                             "dvm_runtime_gc_native_try_leave", "fvm_port_directory_read", "c2string",
                             "dvm_runtime_channel_waiter_published", "dvm_managed_workers_park", "dvm_runtime_prepare_killwait",
                             "dvm_runqueue_complete_channel_wait", "dvm_runqueue_claim", "dvm_managed_workers_yield",
                             "dvm_native_owner_progress", "dvm_gc_workers_create"])
        if "--dedicated-buffer-only" in sys.argv[4:]:
            for name, guest in (("send", SEND_GUEST), ("receive", RECEIVE_GUEST),
                                ("alt-send", ALT_SEND_GUEST), ("alt-receive", ALT_RECEIVE_GUEST),
                                ("alt-ready", ALT_READY_GUEST), ("alt-default", ALT_DEFAULT_GUEST),
                                ("alt-peer", ALT_PEER_GUEST), ("alt-array", ALT_ARRAY_GUEST),
                                ("alt-array-peer", ALT_ARRAY_PEER_GUEST),
                                ("alt-cancel", ALT_STOP_GUEST.replace("chan[8]", "chan[10]"))):
                source, image = work / f"{name}.b", work / f"{name}.dis"
                source.write_text(guest)
                subprocess.run([compiler, "-I", include, "-o", image, source], check=True)
                for mode in ("--no-jit", "--jit"):
                    result = subprocess.run(
                        [binary, mode, "--native-spawn", "--guarded-copies", "--stats", image],
                        cwd=work, capture_output=True, text=True, timeout=5,
                        env=dict(os.environ, DVM_TEST_DEDICATED_BUFFER="1"),
                    )
                    output = result.stdout + result.stderr
                    bypass = name in ("alt-ready", "alt-default")
                    marker = ("PASS ALT bypassed the reservation" if bypass
                              else "PASS foreign channel guest wait")
                    if (result.returncode or output.count(marker) != 1
                            or (bypass and "PASS foreign channel guest wait" in output)
                            or "PASS guest channel payloads" not in output
                            or (name.startswith("alt-") and not bypass
                                and "PASS ALT owns the waiting continuation" not in output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0", output)):
                        raise RuntimeError(f"{mode} dedicated buffered {name}\n{output}")
            return
        if table_kill_head or table_cancel_head:
            source, image = work / "table-kill-head.b", work / "table-kill-head.dis"
            guest = TABLE_FIFO_KILL_HEAD_GUEST.replace(
                "    spawn worker(tab, ids, results, 31);",
                "    for (first := 0; first < 1000; first++) {\n"
                "        (firsttoken, firstdepth, firstwaiting) := tables->guardstate(tab);\n"
                "        if (firsttoken != big 0 && firstdepth == 1 && firstwaiting == 1) break;\n"
                "        sys->sleep(1);\n"
                "    }\n"
                "    spawn worker(tab, ids, results, 31);",
            )
            marker = "PASS guest process termination"
            if table_cancel_head:
                guest = TABLE_CANCEL_GRANTED_GUEST.replace(
                    " != Processctl->Cancelled ||\n"
                    "        control->cancelwait(pid) == Processctl->Cancelled)",
                    " != Processctl->Cancelled)",
                )
                marker = "PASS cancelled table retry"
            source.write_text(guest)
            subprocess.run([compiler, "-I", include,
                            "-I", Path(__file__).resolve().parents[1] / "module",
                            "-o", image, source], check=True)
            for mode in ("--no-jit", "--jit"):
                for attempt in range(8):
                    result = subprocess.run(
                        [binary, mode, "--native-spawn", "--guarded-copies", image],
                        cwd=work, capture_output=True, text=True, timeout=5,
                    )
                    if result.returncode or marker not in result.stdout:
                        raise RuntimeError(f"{mode} table head attempt {attempt + 1}\n"
                                           + result.stdout + result.stderr)
            return
        if table_cancel or "--cancelwait-only" in sys.argv[4:]:
            source, image = work / "cancelwait.b", work / "cancelwait.dis"
            if table_cancel:
                guest = TABLE_CANCEL_GUEST.replace(
                    " != Processctl->Cancelled ||\n"
                    "        control->cancelwait(pid) == Processctl->Cancelled)",
                    " != Processctl->Cancelled)",
                )
                marker = "PASS cancelled table retry"
            else:
                guest = KILLWAIT_CANCEL_GUEST.replace(
                    "control->cancelwait(target) == Processctl->Cancelled || <-reports != 1",
                    "<-reports != 1",
                )
                marker = "PASS guest child retirement"
            source.write_text(guest)
            subprocess.run([compiler, "-I", include, "-I", Path(__file__).resolve().parents[1] / "module",
                            "-o", image, source], check=True)
            for mode in ("--no-jit", "--jit"):
                for attempt in range(8):
                    result = subprocess.run(
                        [binary, mode, "--native-spawn", "--guarded-copies", image],
                        cwd=work, capture_output=True, text=True, timeout=5,
                    )
                    if result.returncode or marker not in result.stdout:
                        raise RuntimeError(f"{mode} {marker[5:]} attempt {attempt + 1} "
                                           f"exited {result.returncode}\n" + result.stdout + result.stderr)
            return
        for operation in ("input <-= 7", "<-input", "alt { <-input => {} <-report => {} }"):
            close_source, close_image = work / "close.b", work / "close.dis"
            close_source.write_text(CLOSE_PARKED_GUEST.replace("WAIT_OPERATION", operation))
            subprocess.run([compiler, "-I", include, "-o", close_image, close_source], check=True)
            close_modes = ("alt-mailbox",) if operation.startswith("alt") else ("direct", "mailbox")
            for close_mode in close_modes:
                for mode in ("--no-jit", "--jit"):
                    closed = subprocess.run(
                        [binary, mode, "--native-spawn", "--guarded-copies", "--stats", close_image],
                        cwd=work, capture_output=True, text=True, timeout=30,
                        env=dict(os.environ, DVM_TEST_CLOSE_PARKED=close_mode),
                    )
                    close_output = closed.stdout + closed.stderr
                    if (closed.returncode or "PASS dedicated channel close" not in close_output
                            or (close_mode == "alt-mailbox" and "TEST posted close to parked ALT" not in close_output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0", close_output)):
                        raise RuntimeError(f"{mode} {close_mode} dedicated close failed\n{close_output}")
        if "--close-only" in sys.argv[4:]:
            print("PASS dedicated SEND, RECV and ALT close with and without JIT")
            return
        (work / "directory-publication").mkdir()
        (work / "directory-publication" / "entry").touch()
        for name, guest in (("send", SEND_GUEST), ("receive", RECEIVE_GUEST),
                            ("failure", FAILURE_GUEST), ("alt-send", ALT_SEND_GUEST),
                            ("alt-receive", ALT_RECEIVE_GUEST), ("alt-failure", ALT_FAILURE_GUEST),
                            ("alt-ready", ALT_READY_GUEST), ("alt-default", ALT_DEFAULT_GUEST),
                            ("alt-empty", ALT_EMPTY_GUEST), ("alt-shared", ALT_SHARED_GUEST),
                            ("alt-shared-retry", ALT_SHARED_RETRY_GUEST), ("alt-distinct", ALT_DISTINCT_GUEST),
                            ("alt-peer", ALT_PEER_GUEST), ("alt-array", ALT_ARRAY_GUEST),
                            ("alt-array-peer", ALT_ARRAY_PEER_GUEST), ("stop-send", STOP_GUEST),
                            ("alt-stop", ALT_STOP_GUEST),
                            ("cancel-send", STOP_GUEST.replace("chan[8]", "chan[10]")),
                            ("alt-cancel", ALT_STOP_GUEST.replace("chan[8]", "chan[10]")),
                            ("cancel-isolated", ISOLATED_CANCEL_GUEST), ("worker-failure", WORKER_FAILURE_GUEST), ("pool-refusal", POOL_REFUSAL_GUEST), ("alt-pool-refusal", ALT_POOL_REFUSAL_GUEST), ("thread-retry", THREAD_RETRY_GUEST), ("alt-thread-retry", ALT_THREAD_RETRY_GUEST), ("io-refusal", IO_REFUSAL_GUEST), ("sleep-cancel", SLEEP_CANCEL_GUEST),
                            ("sleep-worker-failure", SLEEP_WORKER_FAILURE_GUEST),
                            ("sleep-host-handoff", SLEEP_HOST_HANDOFF_GUEST),
                            ("sleep-host-handoff-serial", SLEEP_HOST_HANDOFF_GUEST), ("file-cancel", FILE_CANCEL_GUEST), ("stdin-cancel", STDIN_CANCEL_GUEST),
                            ("stdin-worker-failure", STDIN_WORKER_FAILURE_GUEST), ("stdin-partial", STDIN_PARTIAL_GUEST), ("stdout-cancel", STDOUT_CANCEL_GUEST),
                            ("stdout-partial", STDOUT_PARTIAL_GUEST),
                            ("stdout-worker-failure", STDOUT_WORKER_FAILURE_GUEST), ("stdio-alias", STDOUT_ALIAS_GUEST), ("stdio-alias-cancel", STDOUT_ALIAS_CANCEL_GUEST), ("broken-pipe", BROKEN_PIPE_GUEST),
                            ("broken-pipe-partial", BROKEN_PIPE_PARTIAL_GUEST),
                            ("stdio-alias-protected", STDOUT_ALIAS_PROTECTED_GUEST),
                            ("directory-publication", DIRECTORY_PUBLICATION_GUEST),
                            ("parked-send", PARKED_SEND_GUEST), ("parked-receive", PARKED_RECEIVE_GUEST),
                            ("parked-alt", PARKED_ALT_GUEST), ("parked-array-alt", PARKED_ARRAY_ALT_GUEST),
                            ("parked-isolation", PARKED_ISOLATION_GUEST), ("host-before-park", HOST_CANCEL_GUEST),
                            ("host-after-snapshot", HOST_CANCEL_GUEST.replace("chan[35]", "chan[36]")),
                            ("host-fixed", HOST_CANCEL_GUEST.replace("chan[35]", "chan[44]")),
                            ("host-serial", HOST_CANCEL_GUEST.replace("chan[35]", "chan[46]")),
                            ("processctl", PROCESSCTL_GUEST), ("processctl-inline", PROCESSCTL_INLINE_GUEST), ("process-kill", PROCESS_KILL_GUEST),
                            ("process-kill-native", PROCESS_KILL_NATIVE_GUEST), ("process-kill-ready", PROCESS_KILL_READY_GUEST), ("process-kill-table", PROCESS_KILL_TABLE_GUEST), ("table-fifo", TABLE_FIFO_GUEST), ("table-fifo-move", TABLE_FIFO_MOVE_GUEST), ("process-kill-granted-table", TABLE_FIFO_KILL_HEAD_GUEST),
                            ("table-cancel", TABLE_CANCEL_GUEST), ("table-cancel-granted", TABLE_CANCEL_GRANTED_GUEST),
                            ("killwait-channel", killwait_guest(PROCESS_KILL_GUEST)),
                            ("killwait-native", killwait_guest(PROCESS_KILL_NATIVE_GUEST)),
                            ("killwait-ready", killwait_guest(PROCESS_KILL_READY_GUEST)),
                            ("killwait-table", PROCESS_KILLWAIT_TABLE_GUEST),
                            ("killwait-inline", killwait_guest(PROCESS_KILL_READY_GUEST)),
                            ("killwait-cancel", KILLWAIT_CANCEL_GUEST),
                            ("killwait-nested", KILLWAIT_NESTED_GUEST),
                            ("killwait-host", KILLWAIT_HOST_GUEST),
                            ("killwait-host-fixed", KILLWAIT_HOST_GUEST.replace(
                                "killwait-host.bin", "killwait-host-fixed.bin"))):
            source, image = work / f"{name}.b", work / f"{name}.dis"
            source.write_text(guest)
            subprocess.run([compiler, "-I", include, "-I", Path(__file__).resolve().parents[1] / "module",
                            "-o", image, source], check=True)
            for mode in ("--no-jit", "--jit"):
                workers = "3" if name in ("alt-peer", "alt-array-peer", "cancel-isolated", "worker-failure", "file-cancel", "stdio-alias-cancel", "killwait-cancel", "killwait-nested", "killwait-host-fixed") else "2"
                if name.endswith("-serial"):
                    workers = "1"
                options = ["--managed-workers", workers]
                if name in ("host-before-park", "host-after-snapshot", "killwait-host"):
                    options = ["--managed-workers", "auto", "--stats"]
                if name.endswith("pool-refusal"):
                    options = ["--managed-workers", "auto", "--managed-workers-limit", "2"]
                if name.endswith("thread-retry"):
                    options = ["--managed-workers", "auto", "--stats"]
                if name in ("process-kill-granted-table", "table-cancel-granted"):
                    options += ["--quantum", "65536"]
                if name.startswith(("stdin-", "stdout-", "broken-pipe", "process-kill", "table-cancel", "killwait-")) or name == "directory-publication":
                    options.append("--stats")
                command = [binary, mode, "--guarded-copies", "--gc-worker", *options, image]
                if name == "processctl-inline":
                    command = [binary, mode, "--gc-inline", image]
                if name == "killwait-inline":
                    command = [runtime, mode, "--gc-inline", "--stats", image]
                if name.startswith("stdin-"):
                    result = run_stdin_fixture(command, work, name.endswith("worker-failure"), name == "stdin-partial")
                elif name.startswith("broken-pipe"):
                    result = run_broken_pipe_fixture(command, work, name == "broken-pipe-partial")
                elif name.startswith("stdio-alias"):
                    result = subprocess.run(command, cwd=work, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                            text=True, timeout=30)
                    if (result.returncode or not result.stdout.startswith("a" * 8192 + "bPASS guest channel payloads\n")
                            or "runtime error:" in result.stdout or "ThreadSanitizer:" in result.stdout):
                        raise RuntimeError(f"{name} {mode} changed output order\n{result.stdout[-500:]}")
                    continue
                elif name.startswith("stdout-"):
                    result = run_stdout_fixture(command, work, name.endswith("worker-failure"), name == "stdout-partial")
                elif name in ("stop-send", "alt-stop"):
                    with tempfile.TemporaryFile(mode="w+") as stream:
                        result = subprocess.run(command, cwd=work, stdout=stream, stderr=subprocess.PIPE,
                                                text=True, timeout=30)
                        stream.seek(0)
                        result.stdout = stream.read()
                else:
                    result = subprocess.run(command, cwd=work, capture_output=True, text=True, timeout=30)
                dedicated_markers = {
                    "parked-send": "PASS cancelled scheduler channel wait",
                    "parked-receive": "PASS cancelled scheduler channel wait",
                    "parked-alt": "PASS cancelled scheduler channel wait",
                    "parked-array-alt": "PASS cancelled scheduler channel wait",
                    "processctl": "PASS guest process control",
                    "host-fixed": "PASS guest channel payloads",
                    "killwait-channel": "PASS guest child retirement",
                    "killwait-cancel": "PASS guest child retirement",
                    "killwait-host-fixed": "PASS guest child retirement",
                    "killwait-native": "PASS guest child retirement",
                }
                if name in dedicated_markers:
                    dedicated_image = image
                    if name == "killwait-cancel":
                        # The owner may already have advanced to a different wait
                        # by the time the caller receives its first reply.
                        dedicated_source = work / "killwait-owner-cancel.b"
                        dedicated_image = work / "killwait-owner-cancel.dis"
                        dedicated_source.write_text(guest.replace(
                            "control->cancelwait(target) == Processctl->Cancelled || <-reports != 1",
                            "<-reports != 1",
                        ))
                        subprocess.run([compiler, "-I", include, "-I",
                                        Path(__file__).resolve().parents[1] / "module",
                                        "-o", dedicated_image, dedicated_source], check=True)
                    dedicated = subprocess.run(
                        [binary, mode, "--native-spawn", "--guarded-copies", "--stats", dedicated_image],
                        cwd=work, capture_output=True, text=True, timeout=30,
                        env=dict(os.environ, DVM_TEST_DEDICATED_CANCEL="1") if name == "host-fixed" else None,
                    )
                    native_output = dedicated.stdout + dedicated.stderr
                    starts = re.findall(r"NATIVEGUEST start pid=(\d+) thread=(\d+)", native_output)
                    exits = re.findall(r"NATIVEGUEST retired pid=(\d+) thread=(\d+)", native_output)
                    if (dedicated.returncode or dedicated_markers[name] not in native_output
                            or not starts or sorted(starts) != sorted(exits)
                            or not re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0", native_output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0 migrated=0", native_output)):
                        raise RuntimeError(f"{name} {mode} dedicated wake or retirement failed\n{native_output}")
                if name == "send":
                    resident_command = [binary, mode, "--native-spawn", "--guarded-copies", image]
                    for field in range(15):
                        refused = subprocess.run(resident_command, cwd=work, capture_output=True, text=True,
                                                 timeout=30, env=dict(os.environ, DVM_TEST_NATIVE_RETURN_FIELD=str(field)))
                        if (refused.returncode != 1 or
                                "disvm: native continuation restoration failed" not in refused.stderr):
                            raise RuntimeError(f"{mode} accepted corrupt native return field {field}\n"
                                               + refused.stdout + refused.stderr)
                    for pool in ("2", "auto"):
                        for locked in (False, True):
                            root_options = ["--gc-locked-poll"] if locked else []
                            rooted = subprocess.run(
                                [binary, mode, "--gc-worker", "--guarded-copies",
                                 "--managed-workers", pool, *root_options, image],
                                cwd=work, capture_output=True, text=True, timeout=30,
                                env=dict(os.environ, DVM_TEST_SUSPENDED_ROOTS="1"),
                            )
                            if (rooted.returncode or
                                    "PASS collector preserves suspended continuation roots" not in rooted.stdout):
                                raise RuntimeError(
                                    f"{mode} pool={pool} locked={locked} missed suspended-root check\n"
                                    + rooted.stdout + rooted.stderr
                                )
                    phase_controls = {
                        "nested-leave": "invalid/foreign/nested audited native release",
                        "missing": "missing native heap reentry token",
                        "negative": "invalid native heap reentry token",
                        "noncanonical": "invalid native heap reentry token",
                        "duplicate": "invalid native heap reentry phase",
                    }
                    for phase, diagnostic in phase_controls.items():
                        refused = subprocess.run(
                            resident_command, cwd=work, capture_output=True, text=True, timeout=30,
                            env=dict(os.environ, DVM_TEST_NATIVE_RETURN_PHASE=phase),
                        )
                        if refused.returncode != 1 or diagnostic not in refused.stderr:
                            raise RuntimeError(f"{mode} missed native phase control {phase}\n"
                                               + refused.stdout + refused.stderr)
                output = result.stdout + result.stderr
                bypass = name in ("alt-ready", "alt-default")
                marker = ("PASS ALT bypassed the reservation" if bypass
                          else "PASS foreign channel guest wait")
                expected = 1
                if name == "cancel-isolated":
                    marker = "PASS isolated channel guest wait"
                    expected = 2
                if name.endswith("thread-retry"):
                    marker = "PASS channel wait growth attempt"
                    expected = 2
                    if ("NATIVEPOOL_GROW executors=3 created=2" not in output
                            or "live=0 returning=0" not in output
                            or not re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0", output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0", output)):
                        raise RuntimeError(f"{name} {mode} lifecycle\n{output}")
                if name == "io-refusal":
                    marker = "PASS native I/O admission refusal"
                    expected = 10
                    if (work / "io-refusal.bin").read_bytes() != b"ab" or (work / "refused-create.bin").exists():
                        raise RuntimeError(f"{name} {mode} wrote unexpected file bytes")
                if name == "file-cancel":
                    marker = "PASS cancelled file admission"
                    if (work / "file-cancel.bin").read_bytes() != b"ab":
                        raise RuntimeError(f"{name} {mode} wrote unexpected file bytes")
                if name.startswith("stdin-"):
                    marker = "PASS cancelled stdin poll"
                    if ("NATIVEINPUT pins=0 staging=0" not in output
                            or not re.search(r"NATIVEFILES created=(\d+) closed=\1 live=0 .* pending=0", output)):
                        raise RuntimeError(f"{name} {mode} retained native input resources\n{output}")
                if name.startswith("stdout-"):
                    marker = "PASS cancelled stdout wait"
                    if ("NATIVEOUTPUT pins=0 staging=0" not in output
                            or not re.search(r"NATIVEFILES created=(\d+) closed=\1 live=0 .* pending=0", output)):
                        raise RuntimeError(f"{name} {mode} retained native output resources\n{output}")
                if name.startswith("host-"):
                    marker = "PASS submitted host cancellation batch"
                    if output.count("PASS coalesced cancellation during delivery") != 1:
                        raise RuntimeError(f"{name} {mode} missed concurrent cancellation delivery\n{output}")
                    if name == "host-serial" and output.count("PASS cancellation pending at scheduler yield") != 1:
                        raise RuntimeError(f"{name} {mode} missed the pending cancellation at yield\n{output}")
                    if output.count("PASS guest scheduled between cancellation slices") != 1:
                        raise RuntimeError(f"{name} {mode} skipped the ready guest between slices\n{output}")
                    if name in ("host-before-park", "host-after-snapshot") and "NATIVEPOOL_QUIESCENT processes=1" not in output:
                        raise RuntimeError(f"{name} {mode} did not reach scheduler quiescence\n{output}")
                if name.startswith("parked-"):
                    marker = "PASS cancelled scheduler channel wait"
                if name == "directory-publication":
                    marker = "PASS directory publication retains FIFO"
                    if (not re.search(r"NATIVEMETADATA .* pins=0 staging=0 .* batches=3", output)
                            or not re.search(r"NATIVEFILES created=(\d+) closed=\1 live=0 .* pending=0", output)):
                        raise RuntimeError(f"{name} {mode} retained directory resources\n{output}")
                if name.startswith("broken-pipe"):
                    marker = "PASS closed pipe splice"
                    if ("NATIVEOUTPUT pins=0 staging=0" not in output
                            or not re.search(r"NATIVEFILES created=(\d+) closed=\1 live=0 .* pending=0", output)):
                        raise RuntimeError(f"{name} {mode} retained native output resources\n{output}")
                if name.startswith("sleep-"):
                    marker = "PASS cancelled native sleep"
                    if name.startswith("sleep-host-handoff") and output.count("PASS native cancellation handoff") != 1:
                        raise RuntimeError(f"{name} {mode} missed native cancellation handoff\n{output}")
                if name.startswith("killwait-"):
                    if name in ("killwait-cancel", "killwait-nested", "killwait-host", "killwait-host-fixed") and (work / f"{name}.bin").read_bytes() != b"a":
                        raise RuntimeError(f"{name} {mode} lost admitted child output")
                    if (result.returncode or output.count("PASS guest child retirement") != 1
                            or (name.startswith("killwait-host") and output.count("PASS submitted host cancellation batch") != 1)
                            or (name.endswith("native") and output.count("PASS cancelled native sleep") != 1)
                            or not re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0", output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0", output)
                            or "runtime error:" in output or "ThreadSanitizer:" in output):
                        raise RuntimeError(f"{name} {mode}\n{output}")
                    continue
                if name.startswith("process-kill"):
                    if (result.returncode or output.count("PASS guest process termination") != 1
                            or (name.endswith("native") and output.count("PASS cancelled native sleep") != 1)
                            or (name.endswith("table") and "tables_live=0 keys_live=0" not in output)
                            or not re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0", output)
                            or not re.search(r"NATIVEDISPATCH claims=(\d+) finishes=\1 active=0", output)
                            or "runtime error:" in output or "ThreadSanitizer:" in output):
                        raise RuntimeError(f"{name} {mode}\n{output}")
                    continue
                if name.startswith("table-cancel"):
                    if (result.returncode or output.count("PASS cancelled table retry") != 1
                            or "tables_live=0 keys_live=0" not in output
                            or not re.search(r"NATIVEQUEUE queued=0 leases=(\d+) returns=\1 inflight=0", output)
                            or "runtime error:" in output or "ThreadSanitizer:" in output):
                        raise RuntimeError(f"{name} {mode}\n{output}")
                    continue
                if name.startswith("table-fifo"):
                    if (result.returncode or output.count("PASS cooperative table FIFO") != 1
                            or "runtime error:" in output or "ThreadSanitizer:" in output):
                        raise RuntimeError(f"{name} {mode}\n{output}")
                    continue
                if name.startswith("processctl"):
                    if (result.returncode or output.count("PASS guest process control") != 1
                            or "runtime error:" in output or "ThreadSanitizer:" in output):
                        raise RuntimeError(f"{name} {mode}\n{output}")
                    continue
                failure = name.endswith("worker-failure")
                payload = "fixture fatal worker error" if failure else "PASS guest channel payloads"
                actual = output.count(marker)
                # An unrelated queue publication can wake ALT before the transfer finishes.
                valid_count = actual >= expected if name == "alt-thread-retry" else actual == expected
                if (result.returncode != int(failure) or not valid_count
                        or payload not in output
                        or (name.startswith("alt-") and not bypass
                            and output.count("PASS ALT owns the waiting continuation") != actual)
                        or "ThreadSanitizer:" in output or "runtime error:" in output):
                    raise RuntimeError(f"{name} {mode}\n{output}")
    print("PASS guarded channel contention on native Dis workers in C and JIT modes")


if __name__ == "__main__":
    main()
