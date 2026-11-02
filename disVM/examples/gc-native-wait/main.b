implement NativeWait;
include "sys.m";
sys: Sys; # OS interface loaded by the root process.


# Native-cell example entry; init reports failures through a managed exception.
NativeWait: module
{
    init: fn();
};

Node: adt
{
    link: ref Node; # Self-cycle remains reachable across native waits.
    data: array of int; # Managed payload with a checked sentinel.
};


# Keep arguments, native call frames and a nested managed frame alive while the
# Dis owner releases its heap for a scalar OS wait. The GC must preserve the
# graph, string and array contents even though the C callback is sleeping.
# node/text are caller-owned roots; depth selects the number of nested frames.
# Returns no value and raises fail if any root, local or result is corrupted.
wait(node: ref Node, text: string, depth: int)
{
    local := array[128] of int;
    local[127] = 9000+depth;
    if (depth > 0)
    {
        wait(node,text,depth-1);
    }
    if (sys->sleep(25) != 0 || node.link != node || node.data[0] != 314159 ||
        text != "managed native wait root" || local[127] != 9000+depth)
    {
        raise "fail: native wait lost managed roots";
    }
}


# Normal return checks collector join rather than terminating via test control.
# A negative period is a no-delay success; out-of-native-int periods must refuse.
# No parameters/result; all temporary owners live until normal return or unwind.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil,array[64] of int);
    node.link = node;
    node.data[0] = 314159;
    wait(node,"managed native wait root",3);
    # Construct a wide NC64 word at runtime, avoiding legacy constant narrowing.
    oversized := len node.data;
    oversized <<= 32;
    if (sys->sleep(-1) != 0 || sys->sleep(0) != 0 || sys->sleep(oversized) != -1)
    {
        raise "fail: native sleep scalar range";
    }
    sys->print("PASS native wait: nested managed roots survive released OS sleep\n");
}
