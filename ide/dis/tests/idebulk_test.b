implement BulkPortTest;
include "sys.m";
include "uibridge.m";
include "idehost.m";
include "idebulk.m";

BulkPortTest: module {
    init: fn(); # Exercise real native port admission and sender retirement.
};
sys: Sys; # Borrowed output module for explicit active failures.


# Evaluate a real operation's result in all builds. message names the invariant;
# failure terminates this test process, without leaving a success marker.
require(valid: int, message: string)
{
    if (!valid) {
        sys->print("FAIL private geometry ports: %s\n", message);
        raise "fail: private geometry ports";
    }
}


# Own normal ports until all private senders acknowledge native detachment.
# Invalid/duplicate slots and cross-use of ordinary ports must refuse; a retired
# slot can be safely registered again. No drawing or fake channel mover is used.
init()
{
    sys = load Sys Sys->PATH;
    host := load Idehost Idehost->PATH;
    codec := load Uibridge Uibridge->PATH;
    bulk := load Idebulk Idebulk->PATH;
    require(host != nil && codec != nil && bulk != nil, "load actual modules");
    input := chan of array of byte;
    output := chan of array of byte;
    private := chan of array of byte;
    require(host->attach(input, output) == 0, "normal ports registered");
    require(host->attachbulk(0, private) == -1, "zero slot refused");
    require(host->attachbulk(9, private) == -1, "out of range refused");
    require(host->attachbulk(1, input) == -1, "input cannot become geometry egress");
    require(host->attachbulk(1, output) == -1, "ordinary output remains independent");
    require(host->attachbulk(1, private) == 0, "private registration");
    require(host->attachbulk(1, private) == -1, "duplicate slot refused");
    require(host->attachbulk(2, private) == -1, "duplicate channel refused");
    require(host->attachbulk(1, nil) == 0, "explicit detach");
    completed := chan[8] of int;
    for (round := 0; round < 3; round++) {
        first := bulk->start(codec, host, 1, completed);
        second := bulk->start(codec, host, 2, completed);
        require(first != nil && second != nil, "independent senders start");
        first <-= nil;
        second <-= nil;
        a := <-completed;
        b := <-completed;
        require(a != b && a + b == 3, "both senders release their native borrows");
    }
    host->detach();
    sys->print("PASS private geometry ports: admission, independent owners, repeated retirement\n");
}
