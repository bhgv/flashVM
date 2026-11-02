implement NativeRead;
include "sys.m";
sys: Sys; # Root-owned OS interface.


# Entry for the matching NC64 runtime; stdin is supplied by the companion driver.
NativeRead: module
{
    init: fn();
};

Node: adt
{
    link: ref Node; # Reachable cycle survives blocking native input.
    data: array of int; # Managed sentinel payload.
};


# Raise a managed failure when a scalar/root/buffer condition is false.
# message is borrowed text; no result or transfer of caller ownership.
require(ok: int, message: string)
{
    if (!ok)
    {
        raise "fail: "+message;
    }
}


# Read into a slice while retaining nested managed frames and a cyclic argument.
# node is borrowed from init; result buffers remain local until the call returns.
# Echo the large buffer so the Python driver checks every received byte exactly.
readinput(node: ref Node)
{
    # These files are created in a private directory by the companion driver.
    file := sys->open("input.bin",Sys->OREAD);
    require(file != nil, "regular input open");
    filebuf := array[20] of byte;
    for (f := 0; f < len filebuf; f++)
    {
        filebuf[f] = byte '#';
    }
    require(sys->readn(file,filebuf,len filebuf) == 16, "regular input extent");
    require(string filebuf[:16] == "0123456789abcdef" &&
        string filebuf[16:] == "####", "regular input bytes");
    empty := sys->open("empty.bin",Sys->OREAD);
    require(empty != nil && sys->read(empty,filebuf,20) == 0, "empty file EOF");
    require(sys->read(sys->fildes(1),filebuf,1) == -1, "non-input descriptor");
    writeonly := sys->create("write-only.bin",Sys->OWRITE,8r666);
    require(writeonly != nil, "write-only file create");
    require(sys->readn(writeonly,filebuf,20) == -1 &&
        string filebuf == "0123456789abcdef####", "port error changed buffer");
    fd := sys->fildes(0);
    backing := array[20] of byte;
    for (i := 0; i < len backing; i++)
    {
        backing[i] = byte '#';
    }
    slice := backing[3:15];
    require(sys->read(fd,slice,0) == 0, "zero read");
    require(sys->readn(fd,slice,-1) == 0, "negative readn");
    require(sys->read(fd,nil,1) == 0, "nil buffer");
    sys->print("NATIVE READ ready\n");
    require(sys->readn(fd,slice,8) == 8, "fragmented readn extent");
    require(string slice[:8] == "ABCDEFGH", "slice payload");
    for (i = 0; i < len backing; i++)
    {
        if (i < 3 || i >= 11)
        {
            require(backing[i] == byte '#', "slice boundary overwritten");
        }
    }
    one := array[1] of byte;
    require(sys->read(fd,one,1) == 1 && one[0] == byte 'i', "one read fragment");
    large := array[100003] of byte;
    require(sys->readn(fd,large,len large) == len large, "multi-stage readn extent");
    output := sys->fildes(1);
    sent := 0;
    while (sent < len large)
    {
        written := sys->write(output,large[sent:],len large-sent);
        require(written > 0, "large echo fragment");
        sent += written;
    }
    tail := array[8] of byte;
    for (i = 0; i < len tail; i++)
    {
        tail[i] = byte '#';
    }
    require(sys->readn(fd,tail,len tail) == 4, "EOF partial readn");
    require(string tail[:4] == "tail" && string tail[4:] == "####", "EOF trailing bytes");
    require(sys->read(fd,tail,8) == 0 && sys->readn(fd,tail,8) == 0, "EOF zero");
    require(node.link == node && node.data[0] == 314159, "managed roots lost");
}


# Keep a second real Dis process blocked on a channel while its parent performs
# native I/O. node/answers are caller-owned arguments; local data must survive
# the collector's saved-process root scan. No OS-thread parallelism is claimed.
guardian(node: ref Node, answers: chan of int)
{
    local := array[64] of int;
    local[31] = 4242;
    answers <-= 314159;
    require(local[31] == 4242 && node.link == node && node.data[0] == 314159, "peer roots");
    answers <-= 789;
}


# No parameters/result. Return normally so both GC workers and native TLS retire.
init()
{
    sys = load Sys Sys->PATH;
    node := ref Node(nil,array[64] of int);
    node.link = node;
    node.data[0] = 314159;
    answers := chan of int;
    spawn guardian(node,answers);
    readinput(node);
    require(<-answers == 314159 && <-answers == 789, "peer root acknowledgement");
    sys->print("\nPASS native read: staged slices, exact bytes, partial EOF, rooted native wait\n");
}
