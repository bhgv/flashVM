implement NativeWrite;
include "sys.m";
sys: Sys; # Root-owned OS interface.


# Native-cell entry; the companion driver supplies stdout backpressure.
NativeWrite: module
{
    init: fn();
};

Node: adt
{
    link: ref Node; # Reachable cycle held across native writes.
    data: array of int; # Managed sentinel payload.
};


# Fail the whole managed run when ok is false. message is borrowed; no result
# or ownership transfer occurs, and all errors use the ordinary VM unwind path.
require(ok: int, message: string)
{
    if (!ok)
    {
        raise "fail: "+message;
    }
}


# Write a sliced source while keeping the caller's graph and immutable sentinels
# live. node is borrowed; the driver checks every emitted and saved byte. Returns
# no value and raises on a port status, source mutation or root failure.
writeoutput(node: ref Node)
{
    source := array[100009] of { * => byte 'y' };
    for (i := 0; i < 3; i++)
    {
        source[i] = byte '#';
        source[len source-1-i] = byte '#';
    }
    # Distinguish source offsets and stage edges; include NUL/high bytes.
    source[3] = byte 0;
    source[3+32767] = byte 255;
    source[3+65535] = byte 128;
    source[3+65536] = byte 'Z';
    source[100005] = byte '!';
    slice := source[3:100006];
    output := sys->fildes(1);
    require(sys->write(output,slice,0) == 0, "zero write");
    require(sys->write(output,slice,-1) == 0, "negative write");
    require(sys->write(output,nil,1) == 0, "nil output buffer");
    require(sys->write(sys->fildes(0),slice,1) == -1 &&
        sys->write(sys->fildes(0),slice,0) == -1, "non-output descriptor");
    file := sys->create("output.bin",Sys->OWRITE,8r666);
    require(file != nil, "output file create");
    for (sent := 0; sent < len slice;)
    {
        n := sys->write(file,slice[sent:],len slice-sent);
        require(n > 0, "file write fragment");
        sent += n;
    }
    readonly := sys->open("input.bin",Sys->OREAD);
    require(readonly != nil && sys->write(readonly,slice,1) == -1 &&
        sys->write(readonly,slice,0) == -1, "read-only port error");
    sys->fprint(sys->fildes(2),"NATIVE WRITE ready\n");
    for (sent = 0; sent < len slice;)
    {
        n := sys->write(output,slice[sent:],len slice-sent);
        require(n > 0, "stdout write fragment");
        sent += n;
    }
    require(string source[:3] == "###" && string source[100006:] == "###", "slice boundaries");
    require(node.link == node && node.data[0] == 271828, "write roots");
}


# Keep a second real Dis process blocked on a channel while its parent performs
# native I/O. node/answers are caller-owned arguments; local data must survive
# the collector's saved-process root scan. No OS-thread parallelism is claimed.
guardian(node: ref Node, answers: chan of int)
{
    local := array[64] of int;
    local[31] = 4242;
    answers <-= 271828;
    require(local[31] == 4242 && node.link == node && node.data[0] == 271828, "peer roots");
    answers <-= 789;
}


# Normal return exercises native-root cleanup and worker/TLS joins. No parameters
# or return value; generated files stay inside the companion driver's temp tree.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil,array[64] of int);
    node.link = node;
    node.data[0] = 271828;
    answers := chan of int;
    spawn guardian(node,answers);
    writeoutput(node);
    require(<-answers == 271828 && <-answers == 789, "peer root acknowledgement");
    sys->print("\nPASS native write: immutable slices, exact bytes, rooted backpressure\n");
}
