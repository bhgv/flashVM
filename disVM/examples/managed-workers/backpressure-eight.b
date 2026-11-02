implement ManagedBackpressureEight;
include "sys.m";
sys: Sys; # Borrowed module root used by eight actors under heap exclusion.

ManagedBackpressureEight: module {
    init: fn(); # The companion driver holds stdout until all eight native writers wait.
};

Node: adt {
    next: ref Node; # Cycle shared by all native-call stacks and the parent.
    sentinel: int; # Check after every managed reentry, before using the graph.
};


# Write exactly100003 bytes from a mapped managed array, handling short writes.
# value is one ASCII byte; node is retained through native release/reentry.
# Each syscall copies a bounded snapshot; no managed pointer is read outside heap.
emit(value: int, node: ref Node)
{
    data := array[100003] of {* => byte value};
    output := sys->fildes(1);
    for (sent := 0; sent < len data;) {
        n := sys->write(output,data[sent:],len data-sent);
        if (n <= 0 || node.next != node || node.sentinel != 271828 || int data[0] != value) {
            raise "fail: managed native writer root/snapshot";
        }
        sent += n;
    }
}


# Each lane owns one payload while borrowing the shared cyclic sentinel root.
# The acknowledgement is sent only after every byte reached the native pipe.
child(lane: int, done: chan of int, node: ref Node)
{
    emit('a' + lane, node);
    done <-= lane;
}


# Eight logical actors require eight OS executors to enter native writes while
# the private pipe remains full. The parent waits on a channel and cannot drain
# the pipe. The external driver observes eight leases before reading any byte.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil, 271828);
    node.next = node;
    done := chan of int;
    for (lane := 0; lane < 8; lane++) {
        spawn child(lane, done, node);
    }
    seen := array[8] of int;
    for (completed := 0; completed < 8; completed++) {
        lane := <-done;
        if (lane < 0 || lane >= 8 || seen[lane] || node.next != node) {
            raise "fail: eight native writer completion";
        }
        seen[lane] = 1;
    }
    sys->print("\nPASS managed native overlap\n");
}
