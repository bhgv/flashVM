implement ManagedMetadata;
include "sys.m";
sys: Sys; # Immutable shared interface; native waits retain their original OS owner.

ManagedMetadata: module {
    init: fn(); # Driver supplies files, directory, stdin tokens and native barriers.
};


# Query a UTF-8 path, an open file and an open directory in four separate phases.
# request/ready/done carry copied integers; file/directory remain mapped FD roots.
# The driver pauses the actual native stat/fstat before each result is available.
inspect(file, directory: ref Sys->FD, request, ready, done: chan of int)
{
    for (phase := 0; phase < 4; phase++) {
        if (<-request != phase) {
            raise "fail: metadata phase request";
        }
        ready <-= phase;
        status: int;
        record: Sys->Dir;
        if (phase == 0) {
            (status, record) = sys->stat("metadata-path-Я.bin");
            if (status != 0 || record.name != "metadata-path-Я.bin" || record.length != big 5) {
                raise "fail: metadata native path snapshot";
            }
        } else if (phase == 1) {
            (status, record) = sys->fstat(file);
            if (status != 0 || record.name != "metadata-fd.bin" || record.length != big 0) {
                raise "fail: metadata native file root";
            }
        } else if (phase == 2) {
            (status, record) = sys->fstat(directory);
            if (status != 0 || (record.mode & Sys->DMDIR) == 0) {
                raise "fail: metadata native directory root";
            }
        }
        if (phase == 3) {
            (count, entries) := sys->dirread(directory);
            if (count != 2 || len entries != 2 || entries[0].name == entries[1].name ||
                (entries[0].name != "one" && entries[0].name != "two") ||
                (entries[1].name != "one" && entries[1].name != "two")) {
                raise "fail: metadata native directory batch";
            }
        }
        done <-= phase;
    }
}


# Two enumerators share one directory cursor. Each native batch is independent;
# mapped records/strings remain rooted while channel sends yield between owners.
# A zero tag acknowledges EOF; positive tags transfer one copied filename.
enumerate(directory: ref Sys->FD, names: chan of (int, string))
{
    for (;;) {
        (count, entries) := sys->dirread(directory);
        if (count < 0 || count != len entries) {
            raise "fail: shared directory native batch";
        }
        if (count == 0) {
            names <-= (0, "");
            return;
        }
        for (i := 0; i < count; i++) {
            names <-= (1, entries[i].name);
        }
    }
}


# Parent receives stdin ONLY after the external driver observes the native pause.
# It then converts another UTF-8 path and computes/prints while that pause remains
# held. This handshake proves progress during a wait, without elapsed-time guesses.
init()
{
    sys = load Sys Sys->PATH;
    file := sys->create("metadata-fd.bin", Sys->OWRITE, 8r666);
    directory := sys->open("metadata-dir", Sys->OREAD);
    if (file == nil || directory == nil) {
        raise "fail: metadata descriptor setup";
    }
    request := chan of int;
    ready := chan of int;
    done := chan of int;
    spawn inspect(file, directory, request, ready, done);
    input := sys->fildes(0);
    token := array[1] of byte;
    for (phase := 0; phase < 4; phase++) {
        request <-= phase;
        if (<-ready != phase || sys->read(input, token, 1) != 1 || int token[0] != '0' + phase) {
            raise "fail: metadata native barrier handshake";
        }
        (status, record) := sys->stat("metadata-other-λ.bin");
        if (status != 0 || record.name != "metadata-other-λ.bin" || record.length != big 2) {
            raise "fail: concurrent metadata snapshot";
        }
        value := 0;
        for (i := 0; i < 2000; i++) {
            value += i;
        }
        if (value != 1999000) {
            raise "fail: metadata unrelated compute";
        }
        sys->print("PASS metadata parent phase %d before release\n", phase);
        if (<-done != phase) {
            raise "fail: metadata acknowledgement";
        }
    }
    # Child consumed the native batch; EOF and metadata must share the same gate.
    (count, entries) := sys->dirread(directory);
    if (count != 0 || len entries != 0) {
        raise "fail: directory gate enumeration after metadata";
    }
    (status, record) := sys->fstat(directory);
    if (status != 0 || (record.mode & Sys->DMDIR) == 0) {
        raise "fail: directory gate metadata after enumeration";
    }
    # Cross the32-entry native batch bound with two readers of the SAME FD.
    many := sys->open("metadata-many", Sys->OREAD);
    if (many == nil) {
        raise "fail: shared directory open";
    }
    names := chan of (int, string);
    spawn enumerate(many, names);
    spawn enumerate(many, names);
    seen := array[65] of int;
    completed := 0;
    received := 0;
    while (completed < 2) {
        (tag, name) := <-names;
        if (tag == 0) {
            completed++;
        } else {
            if (len name < 2 || name[0] != 'n') {
                raise "fail: shared directory unexpected name";
            }
            index := int name[1:];
            if (index < 0 || index >= 65 || seen[index]) {
                raise "fail: shared directory lost/duplicate entry";
            }
            seen[index] = 1;
            received++;
        }
    }
    if (received != 65) {
        raise "fail: shared directory entry count";
    }
    sys->print("PASS metadata native waits completed\n");
}
