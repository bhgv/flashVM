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
    service := load Compute Compute->PATH;
    if (service == nil) {
        raise "could not load Compute";
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
