# Copyright (c) 2026 bhgv
# EN: Call the published Compute export with its original typed interface.
# RU: Вызвать опубликованный экспорт Compute через его исходный интерфейс.
implement Client;
include "sys.m";
include "compute.m";
Client: module { init: fn(); };


# EN: Load Compute by its usual path, then check a value or explicit remote error.
# RU: Загрузить Compute по обычному пути и проверить значение или ошибку владельца.
init()
{
    sys := load Sys Sys->PATH;
    service := load Compute Compute->PATH;
    if (service == nil) {
        raise "could not load Compute";
    }
    missing := sys->open("expect-missing", Sys->OREAD) != nil;
    value := 123;
    if (sys->open("expect-error", Sys->OREAD) != nil) {
        value = -7;
    }
    {
        result := service->compute(value);
        if (missing || value < 0 || result != 124) {
            raise "wrong scalar result";
        }
        sys->print("PASS Styx Compute value\n");
    } exception error {
        "remote scalar owner error" =>
            if (value >= 0) {
                raise error;
            }
            sys->print("PASS Styx Compute error\n");
        "remote scalar reply unavailable" =>
            if (!missing) {
                raise error;
            }
            {
                service->compute(0);
                raise "retired route accepted another call";
            } exception {
                "remote scalar route retired" =>
                    ;
            }
            sys->print("PASS Styx Compute missing\n");
    }
}
