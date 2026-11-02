implement ManagedFileOverlap;
include "sys.m";
sys: Sys; # Shared interface; the runtime serializes each native file cursor.

ManagedFileOverlap: module {
    init: fn(); # The driver pauses native fwrite and observes unrelated progress.
};


# Announce readiness, then write one byte using the SAME FD as the other actor.
# fd stays mapped/rooted throughout the native call and FIFO wait. Acknowledge
# only after native completion. No timing delay simulates progress in Limbo.
write(fd: ref Sys->FD, value: int, ready, done: chan of int)
{
    bytes := array[1] of { * => byte value };
    ready <-= value;
    if (sys->write(fd, bytes, 1) != 1) {
        raise "fail: native file overlap write";
    }
    done <-= value;
}


# The test's private interposer blocks the first real fwrite in an OS pipe read.
# The second writer must wait on the per-file FIFO OUTSIDE the managed heap.
# Parent computes/prints before the driver releases that native barrier.
init()
{
    sys = load Sys Sys->PATH;
    fd := sys->create("file-overlap.bin", Sys->OWRITE, 8r666);
    if (fd == nil) {
        raise "fail: native file overlap create";
    }
    ready := chan of int;
    done := chan of int;
    spawn write(fd, 'a', ready, done);
    spawn write(fd, 'b', ready, done);
    first := <-ready;
    second := <-ready;
    if (first + second != 'a' + 'b') {
        raise "fail: native file overlap ready";
    }
    value := 0;
    for (i := 0; i < 2000; i++) {
        value += i;
    }
    if (value != 1999000) {
        raise "fail: unrelated actor computation";
    }
    sys->print("PASS unrelated actor before native file release\n");
    first = <-done;
    second = <-done;
    if (first + second != 'a' + 'b' || sys->seek(fd, big 0, Sys->SEEKRELA) != big 2) {
        raise "fail: native file overlap completion";
    }
    input := sys->open("file-overlap.bin", Sys->OREAD);
    bytes := array[2] of byte;
    if (input == nil || sys->readn(input, bytes, 2) != 2 || int bytes[0] + int bytes[1] != 'a' + 'b') {
        raise "fail: native file overlap readback";
    }
    sys->print("PASS native file overlap completed\n");
}
