# Copyright (c) 2026 bhgv
# EN: Call the original Compute ABI using a published Styx adapter in this VM.
# RU: Вызвать исходный интерфейс Compute через опубликованный адаптер Styx.
implement Client;
include "sys.m";
include "compute.m";
Client: module { init: fn(); };


# EN: Verify a result from the owner's private file or the owner's exception.
# RU: Проверить результат из локального файла владельца либо его исключение.
init()
{
    sys := load Sys Sys->PATH;
    if ((load Compute "/ordinary.dis") != nil) {
        raise "ordinary provider supplied executable adapter";
    }
    service := load Compute Compute->PATH;
    if (service == nil) {
        raise "could not load Compute";
    }
    if (sys->bind("/blocked", "/scalar", Sys->MREPL) < 0) {
        raise "could not rebind caller route";
    }
    fresh := load Compute Compute->PATH;
    if (fresh == nil) {
        raise "could not load second Compute instance";
    }
    {
        fresh->compute(0);
        raise "new instance used old route";
    } exception error {
        "remote scalar call not sent" =>
            ;
    }
    failing := sys->open("expect-error", Sys->OREAD) != nil;
    no_dependency := sys->open("expect-dependency-error", Sys->OREAD) != nil;
    argument := 100;
    if (failing) {
        argument = -1;
    }
    {
        result := service->compute(argument);
        if (failing || no_dependency || result != 142) {
            raise "incorrect owner result";
        }
        sys->print("PASS owner-local Styx Compute value\n");
    } exception error {
        "remote scalar owner error" =>
            if (no_dependency) {
                sys->print("PASS owner-local Styx dependency refusal\n");
            } else if (failing) {
                sys->print("PASS owner-local Styx Compute exception\n");
            } else {
                raise error;
            }
    }
}
