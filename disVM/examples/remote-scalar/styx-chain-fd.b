# Copyright (c) 2026 bhgv
# EN: Check that C has the same caller grants as the preceding library.
# RU: Проверить, что C получил те же разрешения клиента, что и предыдущая библиотека.
implement Chainfd;
include "chain-fd.m";
include "sys.m";


# EN: Check open modes and the shared read cursor of A's original descriptors.
# RU: Проверить права и общую позицию исходных дескрипторов A.
forward(input, output: int): int
{
    sys := load Sys Sys->PATH;
    if (input == -1 && output == -1) {
        parked := sys->open("/.prog/.self/.vfs/chain-parked", Sys->OWRITE);
        data := array[] of {byte 0, byte 0, byte 0, byte 1};
        if (parked == nil || sys->write(parked, data, len data) != len data) {
            raise "cannot report nested library entry";
        }
        sys->sleep(30000);
        raise "cancelled library unexpectedly resumed";
    }
    if (input == -2 && output == -2) {
        return 29;
    }
    if (input == -3 && output == -3) {
        return check_direction(sys, 1);
    }
    if (input == -4 && output == -4) {
        return check_direction(sys, 0);
    }
    if (input <= -10 && output <= -10) {
        return check_styx_pipe(sys, -10 - input, -10 - output);
    }
    inpath := sys->sprint("/.prog/.self/.fd/%d", input);
    outpath := sys->sprint("/.prog/.self/.fd/%d", output);
    in := sys->open(inpath, Sys->OREAD);
    out := sys->open(outpath, Sys->OWRITE);
    data := array[4] of byte;
    if (in == nil || out == nil || sys->open(inpath, Sys->OWRITE) != nil ||
        sys->open(outpath, Sys->OREAD) != nil || sys->readn(in, data, len data) != len data ||
        string data != "live" || sys->write(out, data, len data) != len data) {
        raise "nested library lost descriptor identity, mode, or cursor";
    }
    marker := sys->open("/.prog/.self/.vfs/chain-stage", Sys->OREAD);
    if (marker == nil || sys->readn(marker, data, len data) != len data || data[3] != byte 42) {
        raise "nested library lost VFS";
    }
    chain_readonly_var(sys);
    chain_var(sys, 24, 25);
    return 73;
}

include "chain-var.b";

# EN: Exercise one exchange on two mounted exact fids, without sharing a local FD.
# RU: Проверить обмен на двух точных fid маунта без передачи локального FD.
check_styx_pipe(sys: Sys, first_private, second_private: int): int
{
    to_a := "/.prog/.self/.fd/30";
    from_a := chain_context_path(sys)+"/.fd/31";
    sender := sys->open(to_a, Sys->ORDWR);
    receiver := sys->open(from_a, Sys->ORDWR);
    unrelated := sys->open(from_a, Sys->ORDWR);
    damaged := sys->open(to_a, Sys->ORDWR);
    premature := sys->open(from_a, Sys->ORDWR);
    if (sender == nil || receiver == nil || unrelated == nil || damaged == nil || premature == nil ||
        sys->open(to_a+"/recv", Sys->ORDWR) != nil ||
        sys->open(from_a+"/send", Sys->ORDWR) != nil ||
        sys->open(to_a, Sys->OWRITE) != nil ||
        sys->open(sys->sprint("/.prog/.self/.fd/%d", first_private), Sys->ORDWR) != nil ||
        sys->open(sys->sprint("/.prog/.self/.fd/%d", second_private), Sys->ORDWR) != nil) {
        raise "record pipe exported the wrong endpoint or mode";
    }
    record := pipe_test_record(0);
    malformed := pipe_test_record(0);
    malformed[0] = byte 0;
    receipt := array[] of {byte 1};
    if (sys->write(damaged, malformed, len malformed) >= 0) {
        raise "record pipe accepted damaged checksum";
    }
    if (sys->write(premature, receipt, 1) >= 0) {
        raise "record pipe accepted an unsolicited receipt";
    }
    if (sys->read(sender, receipt, 1) != 0) {
        raise "record pipe reported delivery before submission";
    }
    if (sys->write(sender, record, len record) != len record) {
        raise sys->sprint("record pipe refused valid record: %r");
    }
    pipe_check(pipe_read(sys, sender, 1), array[] of {byte 1});
    pipe_check(pipe_read(sys, receiver, 1024), pipe_test_record(1));
    if (sys->write(unrelated, receipt, 1) >= 0) {
        raise "receipt accepted on a different fid";
    }
    pipe_accept(sys, receiver);
    return 37;
}

include "chain-record-pipe.b";

# EN: Check B's temporary outgoing grant and its successor after the scope ends.
# RU: Проверить временную выдачу B и следующую выдачу после окончания контекста.
check_direction(sys: Sys, changed: int): int
{
    numeric := chain_context_path(sys);
    path := numeric+"/.fd/7";
    if (!changed) {
        chain_var(sys, 25, -1);
        if (sys->open(path, Sys->ORDWR) == nil) {
            raise "temporary B grant replaced A's later FD";
        }
        return 89;
    }
    fd := sys->open(path, Sys->OREAD);
    bytes := array[4] of byte;
    if (fd == nil || sys->open(path, Sys->OWRITE) != nil ||
        sys->open(numeric+"/.fd/8", Sys->OREAD) != nil ||
        sys->readn(fd, bytes, 4) != 4 || bytes[3] != byte 61) {
        raise "C did not get B's scoped numbered FD";
    }
    chain_var(sys, 51, 52);
    return 88;
}

include "chain-context.b";
