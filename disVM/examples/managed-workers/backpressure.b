implement ManagedBackpressure;
include "sys.m";
sys: Sys; # Borrowed module root used by both actors under heap exclusion.

ManagedBackpressure: module {
    init: fn(); # The companion driver holds stdout until both native writers wait.
};

Node: adt {
    next: ref Node; # Cycle shared by both native-call stacks and the parent.
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


# Acknowledge only after this actor's full payload has reached the OS pipe.
# done/node are borrowed mapped roots; the scalar acknowledgement has no pointer.
child(done: chan of int, node: ref Node)
{
    emit('b',node);
    done <-= 1;
}


# Parent and child block in actual writes on DIFFERENT native executors. The
# driver checks a full private pipe and the second native lease before draining.
# No delay in this program simulates overlap; stdout backpressure supplies it.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil,271828);
    node.next = node;
    done := chan of int;
    spawn child(done,node);
    emit('a',node);
    if (<-done != 1 || node.next != node || node.sentinel != 271828) {
        raise "fail: managed native writer completion";
    }
    sys->print("\nPASS managed native overlap\n");
}
