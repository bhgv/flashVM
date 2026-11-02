implement ManagedFile;
include "sys.m";
sys: Sys; # Immutable module reference; all mutable cursors stay under the heap gate.

ManagedFile: module {
    init: fn(); # Write/read a scratch file; the test supplies a private working directory.
};


# Share ONE cursor-bearing FD between two actors. payload/tail are immutable;
# fd remains a mapped argument until every native operation returns. The initial
# pool serializes ordinary files because FVM_PortFile requires external locking.
write(fd: ref Sys->FD, payload, tail: string)
{
    bytes := array of byte payload;
    for (i := 0; i < 64; i++) {
        if (sys->write(fd,bytes,len bytes) != 16 || sys->fprint(fd,"%s",tail) != 1) {
            raise "fail: shared cursor write";
        }
    }
}


# Acknowledge completed file operations only. FD/channel ownership remains with
# the caller; the scheduler may move this actor at complete instruction boundaries.
child(fd: ref Sys->FD, done: chan of int)
{
    write(fd,"bbbbbbbbbbbbbbbb","d");
    done <-= 1;
}


# Use only the tutorial's scratch name. Read after both writers acknowledge;
# count each byte independently instead of depending on actor execution order.
# Native read snapshots and variadic formatting exercise the same file guard.
init()
{
    sys = load Sys Sys->PATH;
    fd := sys->create("managed-shared.bin",Sys->OWRITE,8r666);
    if (fd == nil) {
        raise "fail: shared cursor create";
    }
    done := chan of int;
    spawn child(fd,done);
    write(fd,"aaaaaaaaaaaaaaaa","c");
    if (<-done != 1) {
        raise "fail: shared cursor acknowledgement";
    }
    input := sys->open("managed-shared.bin",Sys->OREAD);
    bytes := array[2176] of byte;
    if (input == nil || sys->readn(input,bytes,len bytes) != len bytes) {
        raise "fail: shared cursor readback";
    }
    counts := array[4] of int;
    for (i := 0; i < len bytes; i++) {
        value := int bytes[i]-'a';
        if (value < 0 || value > 3) {
            raise "fail: shared cursor unexpected byte";
        }
        counts[value]++;
    }
    if (counts[0] != 1024 || counts[1] != 1024 || counts[2] != 64 || counts[3] != 64) {
        raise "fail: shared cursor lost/duplicated bytes";
    }
    sys->print("PASS managed shared cursor: write, fprint and readn serialized\n");
}
