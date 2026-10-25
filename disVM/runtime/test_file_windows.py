"""Run actual Sys seek and channel-owned binary-window requests in both execution modes."""

import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# Compile the readable probe, execute it beside its matching modules, then inspect real bytes.
# argv supplies runner, compiler, header directories and mode. All child processes are bounded.
def main():
    runtime, compiler, sys_headers, ide_headers, mode = sys.argv[1:]
    runtime = str(Path(runtime).resolve())
    compiler = str(Path(compiler).resolve())
    with tempfile.TemporaryDirectory(dir=Path(runtime).parent) as temporary:
        root = Path(temporary)
        payload = root / "resource Я with spaces.bin"
        payload.write_bytes(b"ab\x00cd")
        sparse = root / "sparse.bin"
        with sparse.open("wb") as output:
            output.seek((1 << 31) + 17)
            output.write(b"!")
        written = root / "written.bin"
        source = r"""
implement Probe;
include "sys.m";
include "idefiles.m";

sys: Sys;                          # Native I/O borrowed throughout this test.
files: Idefiles;                    # Actual compiled Dis file-window implementation.
commands: chan of Idefiles->Request; # Independent service owner endpoint.


# Parameterless test entry; fail visibly on any incorrect byte, offset or lifecycle.
Probe: module
{
    init: fn();                     # Native runner entry.
};


# Send one fully initialized request and consume its private reply.
# path/offset/origin/count are borrowed values. Returns a correlated owner response.
window(path: string, offset: big, origin, count: int): Idefiles->Reply
{
    reply := chan of Idefiles->Reply;
    commands <-= Idefiles->Request(42, files->Read, path, offset, origin, count, reply);
    result := <-reply;
    if (result.id != 42) {
        raise "fail: file-window correlation";
    }
    return result;
}


# Exercise genuine Sys and file service behavior; every success path closes its handles.
# No parameters or result; raise a visible failure rather than asserting.
init()
{
    sys = load Sys Sys->PATH;
    files = load Idefiles "@MODULE@";
    if (sys == nil || files == nil) {
        raise "fail: file modules";
    }
    commands = files->start(sys);
    path := @PATH@;
    file := sys->open(path, Sys->OREAD);
    bytes := array[4] of byte;
    if (file == nil || sys->seek(file, big -2, Sys->SEEKEND) != big 3 ||
        sys->readn(file, bytes, 4) != 2 || string bytes[:2] != "cd" ||
        sys->seek(file, big -5, Sys->SEEKRELA) != big 0 ||
        sys->read(file, bytes, 2) != 2 || string bytes[:2] != "ab") {
        raise "fail: actual Sys seek/read";
    }
    minimum := big -9223372036854775807 - big 1;
    maximum := big 9223372036854775807;
    if (sys->seek(file, minimum, Sys->SEEKRELA) != big -1 ||
        sys->seek(file, maximum, Sys->SEEKRELA) != big -1 ||
        sys->seek(file, big 0, 256) != big -1 ||
        sys->seek(file, big 0, Sys->SEEKRELA) != big 2 ||
        sys->read(file, bytes, 1) != 1 || bytes[0] != byte 0 ||
        sys->seek(sys->fildes(1), big 0, Sys->SEEKSTART) != big -1) {
        raise "fail: failed seek changed cursor or accepted stream";
    }
    file = nil;
    file = sys->create(@WRITTEN@, Sys->OWRITE, 8r666);
    if (file == nil || sys->write(file, array of byte "abcd", 4) != 4 ||
        sys->seek(file, big 1, Sys->SEEKSTART) != big 1 ||
        sys->write(file, array of byte "Z", 1) != 1) {
        raise "fail: positioned write";
    }
    file = nil;
    result := window(path, big -3, Sys->SEEKEND, 9);
    if (result.status != files->Ok || result.size != big 5 || result.position != big 2 ||
        len result.bytes != 3 || result.bytes[0] != byte 0 || string result.bytes[1:] != "cd") {
        raise "fail: binary end-relative window";
    }
    result.bytes[0] = byte 99;
    result = window(path, big 2, Sys->SEEKRELA, 1);
    if (result.status != files->Ok || result.bytes[0] != byte 0) {
        raise "fail: response ownership or fresh relative origin";
    }
    result = window(path, big 0, Sys->SEEKSTART, files->MaxBytes);
    if (result.status != files->Ok || len result.bytes != 5 || result.bytes[2] != byte 0) {
        raise "fail: maximum admitted window";
    }
    result = window(path, big 0, 256, 1);
    if (result.status != files->Invalid) {
        raise "fail: invalid origin";
    }
    result = window(path, big 100, Sys->SEEKSTART, 4);
    if (result.status != files->Ok || result.position != big 100 || len result.bytes != 0) {
        raise "fail: EOF window";
    }
    result = window(path, big 0, Sys->SEEKSTART, 0);
    if (result.status != files->Ok || len result.bytes != 0) {
        raise "fail: zero-length window";
    }
    result = window(path, big -1, Sys->SEEKSTART, 1);
    if (result.status != files->Failed || result.bytes != nil || result.position != big -1) {
        raise "fail: negative seek window";
    }
    result = window(path, big 0, Sys->SEEKSTART, files->MaxBytes + 1);
    if (result.status != files->Invalid || result.bytes != nil) {
        raise "fail: window quota";
    }
    result = window(path, big 0, Sys->SEEKSTART, -1);
    if (result.status != files->Invalid) {
        raise "fail: negative byte quota";
    }
    result = window(path + "\0suffix", big 0, Sys->SEEKSTART, 1);
    if (result.status != files->Invalid) {
        raise "fail: NUL path";
    }
    result = window(path + ".missing", big 0, Sys->SEEKSTART, 1);
    if (result.status != files->Failed) {
        raise "fail: missing path";
    }
    for (index := 0; index < 70; index++) {
        result = window(@SPARSE@, big -1, Sys->SEEKEND, 4);
        if (result.status != files->Ok || result.size != big 2147483666 ||
            result.position != big 2147483665 || len result.bytes != 1 || result.bytes[0] != byte '!') {
            raise "fail: big offset or descriptor release";
        }
    }
    reply := chan of Idefiles->Reply;
    # An unanswerable request is ignored without opening a file or blocking shutdown.
    commands <-= Idefiles->Request(98, files->Read, path, big 0, 0, 1, nil);
    commands <-= Idefiles->Request(-1, files->Read, path, big 0, 0, 1, reply);
    result = <-reply;
    if (result.status != files->Invalid || result.id != -1) {
        raise "fail: negative correlation identity";
    }
    commands <-= Idefiles->Request(99, 256, "", big 0, 0, 0, reply);
    result = <-reply;
    if (result.status != files->Invalid || result.id != 99) {
        raise "fail: unknown operation";
    }
    commands <-= Idefiles->Request(100, files->Shutdown, "", big 0, 0, 0, reply);
    result = <-reply;
    if (result.status != files->Ok || result.id != 100) {
        raise "fail: file owner shutdown";
    }
    sys->print("PASS Sys seek and binary file windows\n");
}
"""
        replacements = {
            "@MODULE@": str(Path(runtime).parent / "idefiles.dis"),
            "@PATH@": str(payload),
            "@SPARSE@": str(sparse),
            "@WRITTEN@": str(written),
        }
        for token, value in replacements.items():
            encoded = json.dumps(value, ensure_ascii=False)
            source = source.replace('"' + token + '"', encoded).replace(token, encoded)
        text = root / "probe.b"
        binary = root / "probe.dis"
        text.write_text(source)
        subprocess.run(
            [compiler, "-g", "-I", sys_headers, "-I", ide_headers, "-o", str(binary), str(text)],
            check=True,
            timeout=15,
        )
        result = subprocess.run(
            [runtime, "--jit" if mode == "jit" else "--no-jit", "--stats", str(binary)],
            capture_output=True,
            text=True,
            timeout=30,
        )
        counters = re.search(r"instructions=(\d+) native=(\d+)", result.stderr)
        if (
            result.returncode
            or "PASS Sys seek and binary file windows" not in result.stdout
            or not counters
            or (mode == "jit" and int(counters[2]) == 0)
            or (mode != "jit" and int(counters[2]) != 0)
        ):
            raise RuntimeError(result.stdout + result.stderr)
        if payload.read_bytes() != b"ab\0cd" or written.read_bytes() != b"aZcd":
            raise RuntimeError("binary read changed its input or positioned output was incorrect")
    print("PASS actual Sys positioning, sparse big offsets and channel file windows: " + mode)


if __name__ == "__main__":
    main()
