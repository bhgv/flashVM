implement ManagedWorkers;
include "sys.m";
include "math.m";
math: Math; # Immutable Math interface; supported FP controls belong to each Prog.
sys: Sys; # Shared immutable module reference, accessed under the heap turn.

ManagedWorkers: module {
    init: fn(); # Exercise shared roots, channels, error frames and normal retirement.
};

Node: adt {
    next: ref Node; # Reachable cycle must survive collection and owner migration.
    values: array of int; # Each worker writes its own lane under heap exclusion.
};


# Compute one scalar without native I/O. lane is a positive multiplier; the
# returned sum is deterministic regardless of which OS owner executes a turn.
# Multiple quanta deliberately move live loop/register state between owners.
calculate(lane: int): int
{
    total := 0;
    for (i := 1; i <= 2000; i++) {
        total += lane*i;
    }
    return total;
}


# Retain the caller's graph through computation and a caught nested error.
# node/replies are borrowed mapped arguments; the channel sends copied scalars.
# The runtime, not spawn itself, assigns turns to its configured OS executors.
work(lane: int, node: ref Node, replies: chan of (int,int))
{
    value := calculate(lane);
    caught := 0;
    { raise "worker: handled"; } exception message {
    "worker:*" => caught = 1;
    * => raise message;
    }
    if (!caught || node.next != node) {
        raise "fail: managed worker roots/error frame";
    }
    rounding := Math->RND_Z;
    if (lane == 2) {
        rounding = Math->RND_PINF;
    }
    math->FPcontrol(rounding,Math->RND_MASK);
    token := sys->sprint("lane %d",lane);
    if (sys->werrstr(token) != 0) {
        raise "fail: managed werrstr result slot";
    }
    # The error string belongs to THIS Prog, even after another actor changes
    # its own error and a different OS executor resumes this loop/frame.
    if (calculate(1) != 2001000 || sys->sprint("%r") != token) {
        raise "fail: managed process error state migrated incorrectly";
    }
    if ((math->getFPcontrol() & Math->RND_MASK) != rounding) {
        raise "fail: managed floating rounding state migrated incorrectly";
    }
    node.values[lane] = value;
    replies <-= (lane,value);
}


# Start two channel actors and compute on the parent as well. Default mode
# remains one managed OS executor; --managed-workers 2 enables a second one.
# Bytecode/heap mutation is serialized in both modes, while native I/O may overlap.
init()
{
    sys = load Sys Sys->PATH;
    math = load Math Math->PATH;
    node := ref Node(nil,array[3] of int);
    node.next = node;
    replies := chan of (int,int);
    spawn work(1,node,replies);
    spawn work(2,node,replies);
    parent := calculate(3);
    seen := array[3] of int;
    for (i := 0; i < 2; i++) {
        (lane,value) := <-replies;
        if (lane < 1 || lane > 2 || seen[lane] || value != lane*2001000 || node.values[lane] != value) {
            raise "fail: managed worker channel result";
        }
        seen[lane] = 1;
    }
    if (parent != 6003000 || node.next != node) {
        raise "fail: managed worker parent result";
    }
    sys->print("PASS managed workers: migrated loops, shared roots, channels and caught errors\n");
}
