# Copyright (c) 2026 bhgv
# EN: Real Inferno Styxservers handlers: separate roots, live one-directory reads and stat records.
# RU: Real Styxservers handlers: отдельные roots, live чтение одного каталога и stat records.
implement Check;
include "sys.m";
include "styx.m";
include "styxservers.m";
Check: module { init: fn(); };
sys: Sys; # Loaded builtin.
styx: Styx; # Existing matched protocol codec.
servers: Styxservers; # Existing matched Inferno server framework.
Tmsg, Rmsg: import styx;
Styxserver, Navigator, Navop: import servers;


# EN/RU: Live directory state belongs to one connection navigator, not a serialized tree.
# Live state принадлежит navigator одного connection, не serialized tree.
State: adt
{
    root: big; # Distinct root qid for this connection.
    user: string; # Original attach user's visible scope.
    phase: int; # Adds a new directory entry after the first read.
    walks: int; # Actual name navigation calls.
    reads: int; # Actual single-directory enumeration calls.
};


# EN/RU: Fail observed contract explicitly.
# Явно завершить test при нарушении observed contract.
check(ok: int, text: string)
{
    if (!ok) {
        raise text;
    }
}


# EN/RU: Produce current attributes for one node, never enumerate descendants.
# Вернуть current attributes одного node, не перечисляя descendants.
metadata(s: ref State, index: int): ref Sys->Dir
{
    d := ref Sys->zerodir;
    d.uid = s.user;
    d.gid = s.user;
    d.muid = s.user;
    d.atime = 11;
    d.mtime = 22 + s.phase;
    d.length = big 3;
    d.qid = Sys->Qid(s.root + big index, s.phase, Sys->QTFILE);
    d.mode = 8r444;
    if (s.user == "bob") {
        d.mode = 8r666;
    }
    case index {
    0 => d.name = "/";
    1 => d.name = "first";
    2 => d.name = "sub";
    3 => d.name = "later-λ";
    }
    if (index == 0 || index == 2) {
        d.qid.qtype = Sys->QTDIR;
        d.mode = Sys->DMDIR | 8r555;
        d.length = big 0;
    }
    return d;
}


# EN/RU: Serve requested node/directory only; unknown names and outside paths refuse.
# Обслужить только requested node/directory; unknown/outside paths отклонить.
navigate(s: ref State, input: chan of ref Navop, done: chan of int)
{
    for (;;) {
        pick m := <-input {
        Stat =>
            if (m.path == big -1) {
                m.reply <-= (nil, nil);
                done <-= 1;
                return;
            }
            index := int (m.path - s.root);
            if (index < 0 || index > 2 + s.phase) {
                m.reply <-= (nil, "not found");
            } else {
                m.reply <-= (metadata(s, index), nil);
            }
        Walk =>
            s.walks++;
            if (m.path != s.root) {
                m.reply <-= (nil, "not a directory");
            } else {
                index := -1;
                case m.name {
                ".." => index = 0;
                "first" => index = 1;
                "sub" => index = 2;
                "later-λ" => if (s.phase) { index = 3; }
                }
                if (index < 0) {
                    m.reply <-= (nil, "not found");
                } else {
                    m.reply <-= (metadata(s, index), nil);
                }
            }
        Readdir =>
            check(m.path == s.root, "recursive directory enumeration");
            s.reads++;
            for (i := m.offset; i < 2 + s.phase && i < m.offset + m.count; i++) {
                m.reply <-= (metadata(s, i + 1), nil);
            }
            m.reply <-= (nil, nil);
        }
    }
}


# EN/RU: Round-trip actual Rread wire bytes, checking complete stat records and attributes.
# Round-trip actual Rread wire bytes; проверить complete stat records/attributes.
listing(srv: ref Styxserver, s: ref State, expected: int)
{
    check(srv.read(ref Tmsg.Read(20, 1, big 0, 4096)) != nil, "directory read handler");
    raw := <-srv.replychan;
    packed := raw.pack();
    (used, decoded) := Rmsg.unpack(packed);
    check(used == len packed, "Rread wire extent");
    pick m := decoded {
    Read =>
        count := 0;
        seen := 0;
        names := array[] of {"first", "sub", "later-λ"};
        for (offset := 0; offset < len m.data;) {
            (n, d) := styx->unpackdir(m.data[offset:]);
            check(n > 0 && offset + n <= len m.data && d.uid == s.user && d.gid == s.user &&
                  d.muid == s.user && d.atime == 11 && d.mtime == 22 + s.phase &&
                  d.qid.path > s.root && d.qid.path <= s.root + big expected, "stat record attributes");
            index := int (d.qid.path - s.root);
            check(d.name == names[index - 1] && (seen & (1 << index)) == 0, "decoded name/qid identity");
            seen |= 1 << index;
            if (d.name == "sub") {
                check((d.mode & Sys->DMDIR) != 0 && d.qid.qtype == Sys->QTDIR, "subdirectory stat");
            } else {
                check(d.length == big 3 && (d.mode & 8r222) == (8r222 * (s.user == "bob")), "file mode/length");
            }
            offset += n;
            count++;
        }
        check(count == expected && seen == (1 << (expected + 1)) - 2, "one-directory membership");
    * => raise "directory read did not return Rread";
    }
}


# EN/RU: Build a connection-local handler instance without claiming a socket listener.
# Создать connection-local handler instance; socket listener этим не реализуется.
connection(s: ref State, done: chan of int): ref Styxserver
{
    nav := ref Navigator(chan of ref Navop, chan of (ref Sys->Dir, string));
    spawn navigate(s, nav.c, done);
    srv := ref Styxserver(nil, array[32] of list of ref Styxservers->Fid, chan[1] of int,
                         nav, s.root, 8192, chan[1] of ref Rmsg);
    check(srv.attach(ref Tmsg.Attach(1, 1, Styx->NOFID, s.user, "requested")) != nil, "attach prepared root");
    pick reply := <-srv.replychan {
    Attach => check(reply.qid.path == s.root, "wrong connection root");
    * => raise "attach response";
    }
    return srv;
}


# EN/RU: Exercise original handlers and codec in normal native guest execution.
# Проверить original handlers/codec в normal native guest execution.
init()
{
    sys = load Sys Sys->PATH;
    styx = load Styx "styx.dis";
    servers = load Styxservers "styxservers.dis";
    check(sys != nil && styx != nil && servers != nil, "load Styx modules");
    styx->init();
    servers->init(styx);
    done := chan of int;
    alice := ref State(big 100, "alice", 0, 0, 0);
    bob := ref State(big 200, "bob", 1, 0, 0);
    a := connection(alice, done);
    b := connection(bob, done);
    check(alice.reads == 0 && bob.reads == 0, "attach enumerated tree");
    names := array[] of {"first"};
    check(a.walk(ref Tmsg.Walk(2, 1, 2, names)) != nil && b.walk(ref Tmsg.Walk(2, 1, 2, names)) != nil,
          "walk current child");
    <-a.replychan;
    <-b.replychan;
    check(alice.reads == 0 && bob.reads == 0, "Twalk enumerated directory");
    check(a.walk(ref Tmsg.Walk(5, 1, 9, array[0] of string)) != nil, "clone root fid");
    <-a.replychan;
    check(a.walk(ref Tmsg.Walk(11, 9, 6, array[] of {".."})) != nil && a.getfid(6).path == alice.root,
          "parent walk escaped prepared root");
    <-a.replychan;
    later := array[] of {"later-λ"};
    check(a.walk(ref Tmsg.Walk(6, 9, 4, later)) == nil, "missing node existed before change");
    <-a.replychan;
    check(a.open(ref Tmsg.Open(3, 2, Sys->OWRITE)) == nil, "readonly connection writable");
    <-a.replychan;
    check(b.open(ref Tmsg.Open(3, 2, Sys->OWRITE)) != nil, "writable connection refused");
    <-b.replychan;
    check(a.open(ref Tmsg.Open(4, 1, Sys->OREAD)) != nil && b.open(ref Tmsg.Open(4, 1, Sys->OREAD)) != nil,
          "open root directories");
    <-a.replychan;
    <-b.replychan;
    walks := alice.walks;
    listing(a, alice, 2);
    listing(b, bob, 3);
    alice.phase = 1;
    listing(a, alice, 3);
    check(alice.walks == walks && bob.walks == 1, "directory Tread performed implicit walk");
    check(a.walk(ref Tmsg.Walk(7, 9, 4, later)) != nil, "fresh walk missed live directory change");
    <-a.replychan;
    check(a.open(ref Tmsg.Open(8, 2, Sys->OREAD)) != nil, "open readable file");
    <-a.replychan;
    read := ref Tmsg.Read(9, 2, big 0, 3);
    (fid, err) := a.canread(read);
    check(fid != nil && err == nil && alice.walks == walks + 1, "file Tread required another walk");
    bytes := servers->readbytes(read, array of byte "ABC");
    check(string bytes.data == "ABC", "file read helper");
    check(a.walk(ref Tmsg.Walk(10, 2, 5, names)) == nil, "walk accepted already-open fid");
    pick reply := <-a.replychan {
    Error => check(reply.ename == Styxservers->Eopen, "wrong opened-fid walk error");
    * => raise "walk on opened fid response";
    }
    check(a.getfid(2).path == big 101 && b.getfid(2).path == big 201, "fid crossed connection boundary");
    check(a.t.stat(big -1).t0 == nil && b.t.stat(big -1).t0 == nil, "stop navigators");
    check(<-done == 1 && <-done == 1, "navigator completion");
    sys->print("PASS Styx live roots attach walk directory stat permissions independent fids\n");
}
