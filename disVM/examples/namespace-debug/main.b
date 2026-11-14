# Copyright (c) 2026 bhgv
# EN: Retained shared/copied namespaces inspected through independent debugger pipes.
# RU: Retained shared/copied namespaces для исследования через отдельные debug pipes.
implement NamespaceDebug;
include "sys.m";
include "processctl.m";
NamespaceDebug: module { init: fn(); };
sys: Sys; # Shared immutable builtin interface.
ctl: Processctl; # PID and child lifetime control.


# EN/RU: Require scalar result; raise ordinary managed error on refusal.
# Проверить scalar result; обычная managed ошибка при отказе.
require(value: int, label: string)
{
    if (!value) { raise label; }
}


# EN: Optionally detach/chdir before publishing PID and raising first-chance error.
# ready transfers only PID; debugger retains the original frame/group until release.
# RU: Опционально отделить/chdir до PID и first-chance error. ready передаёт PID;
# debugger удерживает исходный frame/group до release.
held(copy: int, ready: chan of int)
{
    if (copy) {
        require(sys->pctl(Sys->FORKNS, nil) > 0 && sys->chdir("visible") == 0, "namespace copy failed");
    }
    ready <-= ctl->pid();
    raise "namespace debug retained";
}


# EN/RU: Read explicit host permit from ordinary stdin; debugger uses separate pipes.
# Читать host permit из обычного stdin; debugger использует отдельные pipes.
permit()
{
    data := array[1] of byte;
    require(sys->read(sys->fildes(0), data, 1) == 1, "namespace debug permit missing");
}


# EN: Install A, retain shared/copied faults, then rebind to B on explicit permit.
# The shared frozen owner sees group mutation; copied owner keeps A/cwd unchanged.
# RU: Установить A, удержать shared/copied faults, затем по permit связать B.
# Shared frozen owner видит mutation группы, copied сохраняет A/cwd.
init()
{
    sys = load Sys Sys->PATH;
    ctl = load Processctl Processctl->PATH;
    require(sys->bind("source-a", "visible", Sys->MREPL) == 0, "namespace initial bind failed");
    ready := chan of int;
    spawn held(0, ready);
    shared := <-ready;
    spawn held(1, ready);
    copied := <-ready;
    sys->print("NSDEBUG main=%d shared=%d copied=%d\n", ctl->pid(), shared, copied);
    permit();
    require(sys->bind("source-b", "visible", Sys->MREPL) == 0, "namespace rebind failed");
    sys->print("NSCHANGED shared=B copied=A\n");
    permit();
    sys->print("NSDONE\n");
}
