# Copyright (c) 2026 bhgv
# EN: An ordinary unbound dependency inherits the invoking instance's native launch metadata.
# RU: Обычная unbound dependency наследует native launch metadata вызывающего instance.
implement Value;
include "value.m";
include "launchcontext.m";


# EN/RU: Check inherited metadata through the actual builtin before returning the compiled dependency value.
# Проверить inherited metadata через actual builtin до возврата compiled dependency value.
value(): int
{
    launch := load Launchcontext Launchcontext->PATH;
    if (launch == nil || launch->name() != "/app.dis") {
        raise "unbound dependency lost instance label";
    }
    arguments := launch->arguments();
    if (len arguments != 3 || int arguments[0] != 77 || int arguments[1] != 0 ||
        int arguments[2] < 1 || int arguments[2] > 2) {
        raise "unbound dependency read process arguments";
    }
    return EXPECTED_VALUE;
}
