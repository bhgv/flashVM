implement GroupProbe;
include "sys.m";
GroupProbe: module { init: fn(); };
Node: adt { left, right: ref Node; };
make()
{
    a := ref Node(nil, nil);
    b := ref Node(a, nil);
    c := ref Node(a, b);
    a.left = b;
    a.right = c;
    b.right = c;
}
init()
{
    sys := load Sys Sys->PATH;
    live := ref Node(nil, nil);
    alias := ref Node(live, live);
    live.left = alias;
    for(i := 0; i < 128; i++)
        make();
    sys->sleep(30);
    if(live.left != alias || alias.left != live || alias.right != live)
        raise "live group changed";
    sys->print("PASS live group\n");
}
