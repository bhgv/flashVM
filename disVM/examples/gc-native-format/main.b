implement NativeFormat;
include "sys.m";
sys: Sys; # Borrowed system module retained by the entry process.


# Entry contract for the matching native-cell SDK; no external callbacks.
NativeFormat: module
{
    init: fn();
};


# Caller-owned cyclic data whose reachability must survive released native I/O.
Node: adt
{
    link: ref Node; # Cycle retained through blocked formatted output.
    data: array of int; # Sentinel checked after all native returns.
};


# Raise a managed failure unless ok is true. message is borrowed diagnostic text;
# no result or transfer. This also exercises the ordinary host unwind/join path.
require(ok: int, message: string)
{
    if (!ok)
    {
        raise "fail: "+message;
    }
}


# Keep a separate channel-blocked process's locals in the GC root set. node and
# answers are borrowed process arguments. No OS-parallel Dis execution is claimed.
guardian(node: ref Node, answers: chan of int)
{
    local := array[32] of int;
    local[17] = 789;
    answers <-= 271828;
    require(local[17] == 789 && node.link == node, "peer roots");
    answers <-= 789;
}


# Build managed strings/variadic arguments, then block real stdout with both
# print and fprint. The Python driver checks every UTF-8 output byte and file
# result. node is borrowed and stays live; all file handles retire normally.
output(node: ref Node)
{
    source := array[2048] of { * => byte 'y' };
    text := "Ж:"+string source;
    file := sys->create("formatted.txt",Sys->OWRITE,8r666);
    require(file != nil, "create");
    readonly := sys->open("input.txt",Sys->OREAD);
    require(readonly != nil, "open");
    require(sys->fprint(readonly,"not written") == -1, "readonly status");
    require(sys->fprint(readonly,"") == -1, "readonly zero status");
    require(sys->fprint(sys->fildes(0),"not written") == -1, "stdin status");
    stderr := sys->fildes(2);
    stdout := sys->fildes(1);
    sys->fprint(stderr,"NATIVE FORMAT ready\n");
    for (i := 0; i < 64; i++)
    {
        # Each formatted result fits the existing 4096-byte contract. Its UTF-8
        # byte count differs from the managed rune length and is checked as such.
        expected := len array of byte (text+":"+string i+"\n");
        require(sys->print("%s:%d\n",text,i) == expected, "print byte count");
        require(sys->fprint(stdout,"%s:%d\n",text,i) == expected, "fprint byte count");
        require(sys->fprint(file,"row:%d:Ж\n",i) > 0, "file output");
    }
    require(string source == string array[2048] of { * => byte 'y' }, "source unchanged");
    require(node.link == node && node.data[0] == 271828, "caller roots");
}


# Run nested managed roots and normal return with exact collector/TLS cleanup.
# No arguments/result. All files belong to the companion driver's private tree.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil,array[64] of int);
    node.link = node;
    node.data[0] = 271828;
    answers := chan of int;
    spawn guardian(node,answers);
    output(node);
    require(<-answers == 271828 && <-answers == 789, "peer acknowledgement");
    sys->print("PASS native format: exact UTF-8, saved roots, backpressure\n");
}
