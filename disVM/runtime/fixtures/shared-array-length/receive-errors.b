# Copyright (c) 2026 bhgv
# EN: Extracted raw acceptance oracle; preserve instruction order and literals.
# RU: Извлечённый raw-oracle приёмки; сохранить порядок и литералы.
implement ReceiveErrors;
include "sys.m";
ReceiveErrors: module {init: fn();};

sender(records: chan of (int, string), ready, done: chan of int, tag: int) {
    ready <-= 1;
    {
        records <-= (tag, "queued sender");
        done <-= 1;
    } exception failure {
        "channel transfer failed" => done <-= -1;
        * => raise;
    }
}

init() {
    sys := load Sys Sys->PATH;
    records := chan[2] of (int, string);
    records <-= (11, "first");
    records <-= (22, "second");
    value: (int, string);
    {
        value = <-records;
        raise "expected precommit failure";
    } exception failure {
        "receive failed before publication" => ;
        * => raise;
    }
    {
        value = <-records;
        raise "expected postcommit failure";
    } exception failure {
        "receive failed after publication" => ;
        * => raise;
    }
    (tag, text) := <-records;
    if (tag != 22 || text != "second")
        raise "receive failure corrupted the ring";
    for (cycle := 0; cycle < 10; cycle++) {
        if (cycle == 0) {
            {
                records <-= (cycle, "refill");
                raise "expected committed send failure";
            } exception failure {
                "send failed after publication" => ;
                * => raise;
            }
        } else
            records <-= (cycle, "refill");
        records <-= (cycle+100, "wraparound");
        (tag, text) = <-records;
        if (tag != cycle || text != "refill")
            raise "receive failure prevented slot reuse";
        (tag, text) = <-records;
        if (tag != cycle+100 || text != "wraparound")
            raise "receive failure corrupted wraparound order";
    }
    records <-= (44, "first queued");
    records <-= (45, "second queued");
    ready := chan[1] of int;
    done := chan[1] of int;
    spawn sender(records, ready, done, 55);
    if (<-ready != 1)
        raise "sender did not start";
    if (sys->sleep(42421) != 0)
        raise "queued sender handshake failed";
    {
        value = <-records;
        raise "expected refill failure";
    } exception failure {
        "refill failed after publication" => ;
        * => raise;
    }
    if (<-done != 1)
        raise "committed sender did not complete";
    (tag, text) = <-records;
    if (tag != 45 || text != "second queued")
        raise "refill changed existing message order";
    (tag, text) = <-records;
    if (tag != 55 || text != "queued sender")
        raise "refill lost the published sender";
    records <-= (66, "first before failure");
    records <-= (67, "second before failure");
    spawn sender(records, ready, done, 55);
    if (<-ready != 1)
        raise "second sender did not start";
    if (sys->sleep(42421) != 0)
        raise "second queued sender handshake failed";
    {
        value = <-records;
        raise "expected uncommitted refill failure";
    } exception failure {
        "refill failed before publication" => ;
        * => raise;
    }
    if (<-done != -1)
        raise "uncommitted sender reported success";
    (tag, text) = <-records;
    if (tag != 67 || text != "second before failure")
        raise "uncommitted refill corrupted the ring";
    records <-= (68, "after failed refill");
    (tag, text) = <-records;
    if (tag != 68 || text != "after failed refill")
        raise "failed refill left a phantom message";
    sys->print("PASS buffered receive commit boundary\n");
}
