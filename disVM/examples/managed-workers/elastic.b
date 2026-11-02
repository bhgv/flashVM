implement ManagedElastic;
include "sys.m";
sys: Sys; # Borrowed module root used by arbitrary logical actors under heap exclusion.

ManagedElastic: module {
    init: fn(); # The companion driver holds stdout until twelve native writers wait.
};

Node: adt {
    next: ref Node; # Cycle shared by all native-call stacks and the parent.
    sentinel: int; # Check after every managed reentry, before using the graph.
};


# Write one65536-byte snapshot with a lane marker and zero-filled tail.
# value is one ASCII byte; node is retained through native release/reentry.
# Each syscall copies a bounded snapshot; no managed pointer is read outside heap.
emit(value: int, node: ref Node)
{
    data := array[65536] of {* => byte 0};
    data[0] = byte value;
    output := sys->fildes(1);
    for (sent := 0; sent < len data;) {
        n := sys->write(output,data[sent:],len data-sent);
        if (n <= 0 || node.next != node || node.sentinel != 271828 || int data[0] != value) {
            raise "fail: managed native writer root/snapshot";
        }
        sent += n;
    }
}


# A logical channel listener parks without owning an OS executor. Each task
# exits after one message; sum/replies carry copied scalars and no native handle.
listen(input: chan of int, replies: chan of int)
{
    replies <-= <-input;
}


# A blocked constant send must retain frame-owned data even after its previous
# OS executor retires and a later pressure wave reuses native context storage.
publish(literals: chan of int)
{
    literals <-= 73;
}


# Each native writer keeps its own bounded snapshot and acknowledges full output.
# Native waits pin OS owners; the automatic pool must create replacements.
writer(lane: int, node: ref Node, done: chan of int)
{
    emit('a' + lane, node);
    done <-= lane;
}


# Run one native-pressure wave. Completion channels contain only scalar lanes;
# caller retains the shared sentinel through all native operations and joins.
burst(node: ref Node)
{
    done := chan of int;
    for (lane := 0; lane < 12; lane++) {
        spawn writer(lane, node, done);
    }
    # Explicit initialization makes repeated waves independent of recycled storage.
    seen := array[12] of {* => 0};
    for (i := 0; i < 12; i++) {
        lane = <-done;
        if (lane < 0 || lane >= 12 || seen[lane]) {
            raise "fail: elastic writer completion";
        }
        seen[lane] = 1;
    }
}


# Create256 channel listeners and two waves of12 blocked native writers. The
# driver observes each wave, drains exact bytes, then waits for actual idle joins
# BEFORE releasing stdin. No wall-clock sleep establishes lifecycle success.
init()
{
    sys = load Sys Sys->PATH;
    input := chan of int;
    replies := chan of int;
    literals := chan of int;
    for (i := 0; i < 256; i++) {
        spawn listen(input, replies);
    }
    for (i = 0; i < 64; i++) {
        spawn publish(literals);
    }
    node := ref Node(nil, 271828);
    node.next = node;
    token := array[1] of byte;
    for (wave := 0; wave < 2; wave++) {
        sys->print("ELASTIC_BEGIN\n");
        burst(node);
        sys->print("ELASTIC_IDLE\n");
        if (sys->read(sys->fildes(0), token, 1) != 1 || int token[0] != '1') {
            raise "fail: elastic idle handshake";
        }
        if (wave == 0) {
            # Wake every logical task after excess OS helpers were already joined.
            for (i = 0; i < 256; i++) {
                input <-= i;
                if (<-replies != i) {
                    raise "fail: elastic listener wake/result";
                }
            }
        }
    }
    for (i = 0; i < 64; i++) {
        if (<-literals != 73) {
            raise "fail: blocked constant send survived executor reuse";
        }
    }
    if (node.next != node || node.sentinel != 271828) {
        raise "fail: elastic cyclic roots";
    }
    sys->print("PASS elastic listeners256 writers12 waves2\n");
}
