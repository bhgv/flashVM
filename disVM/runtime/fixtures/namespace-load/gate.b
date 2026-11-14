# Copyright (c) 2026 bhgv
# EN: Observe frozen native opens while parent rebinds and host replaces an image.
# RU: Проверить frozen native open при rebind родителя и host-замене образа.
implement NamespaceLoadGate;
include "sys.m";
include "processctl.m";
include "provider.m";
NamespaceLoadGate: module { init: fn(); };
sys: Sys; # Builtin loaded before native child creation.
control: Processctl; # Exact native lifetime replies.
Warm: con 1; # Driver also compiles cold lookup using a separately named retained image.


# EN/RU: Hold the first opened provider across rebind, then load the new namespace route.
# Держать первый provider через rebind, затем загрузить новый namespace route.
victim(done: chan of int)
{
    first := load NamespaceProvider "visible/provider.dis";
    next := load NamespaceProvider "visible/provider.dis";
    if (first == nil || next == nil || first->tag() != 11 || next->tag() != 22) {
        raise "frozen module input changed or next namespace route lost";
    }
    done <-= control->pid();
}


# EN: Read two controller acknowledgments only after genuine open/rebind milestones.
# Keep the old Module alive while a same-path atomic host replacement is loaded.
# The stale value is reported without throwing so omission tests retire normally.
# RU: Читать два controller ack после настоящих open/rebind milestones. Держать
# старый Module при загрузке same-path host replacement. Устаревшее значение
# выводится без throw, чтобы omission-тест завершал обычный retirement.
init()
{
    sys = load Sys Sys->PATH;
    control = load Processctl Processctl->PATH;
    oldpath := "source-a/provider.dis";
    if (!Warm) {
        oldpath = "source-a/retained.dis";
    }
    old := load NamespaceProvider oldpath;
    if (old == nil || old->tag() != 11 || sys->bind("package/source-a", "package/visible", Sys->MREPL) != 0) {
        raise "namespace gate setup failed";
    }
    done := chan of int;
    spawn victim(done);
    input := sys->fildes(0);
    bytebuf := array[1] of byte;
    if (sys->read(input, bytebuf, 1) != 1 ||
        sys->bind("package/source-b", "package/visible", Sys->MREPL) != 0) {
        raise "namespace gate rebind failed";
    }
    peer := load NamespaceProvider "visible/provider.dis";
    if (peer == nil || peer->tag() != 22) {
        raise "namespace peer could not load during held native input";
    }
    sys->print("NAMESPACELOADREBIND peer=22\n");
    if (sys->read(input, bytebuf, 1) != 1) {
        raise "namespace replacement acknowledgment missing";
    }
    fresh := load NamespaceProvider "source-a/provider.dis";
    if (fresh == nil || old->tag() != 11) {
        raise "namespace replacement lost retained module";
    }
    sys->print("NAMESPACELOADFRESH value=%d\n", fresh->tag());
    pid := <-done;
    result := control->killwait(pid);
    if (result != Processctl->Retired && result != Processctl->Denied) {
        raise "namespace gate child retained";
    }
    sys->print("NAMESPACELOADGATE first=11 next=22 old=11 fresh=%d\n", fresh->tag());
}
