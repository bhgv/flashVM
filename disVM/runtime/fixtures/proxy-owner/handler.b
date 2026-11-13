# Copyright (c) 2026 bhgv
# EN: Actual Limbo callback with original refs and permanent-owner identity checks.
# RU: Настоящий Limbo callback с original refs и проверкой постоянного owner.
implement Proxyhandler;
include "sys.m";
include "processctl.m";
include "proxyhost.m";
include "proxyhandler.m";
include "proxyowner.m";


# EN/RU: Store this serving Prog identity in its original captured cell.
# Сохранить identity обслуживающего Prog в исходной captured cell.
start(capture: ref Proxyhost->Cell)
{
    control := load Processctl Processctl->PATH;
    capture.value = control->pid();
}


# EN: Run on the original owner; mutate actual alias refs, optionally delay/raise.
# -999 publishes entry then blocks for retirement; -2 waits for the original-cell gate,
# -1 raises after a partial effect. All arguments remain borrowed rooted refs.
# RU: Исполнить на исходном owner; менять actual aliases, при необходимости delay/raise.
# -999 сообщает entry и блокируется для retirement, -2 ждёт gate в исходной cell,
# -1 бросает исключение после частичного effect. Аргументы — borrowed rooted refs.
invoke(capture, left, right: ref Proxyhost->Cell, value: int): int
{
    control := load Processctl Processctl->PATH;
    sys := load Sys Sys->PATH;
    if (capture.value != control->pid() || left == nil || left != right) {
        raise "proxy callback lost owner or ref alias";
    }
    if (value == -4) {
        nested := load Proxyowner Proxyowner->PATH;
        nested->init();
        empty := Proxyowner->Handle(big 0, 0, big 0);
        endpoint := ref Proxyowner->Endpoint(chan of ref Proxyowner->Request);
        (status, unused) := nested->call(endpoint, Proxyowner->Read, empty, empty, empty, 0, 0, 1000);
        if (status != Proxyowner->OwnerError) {
            raise "active callback entered a blocking proxy wait";
        }
        return 7777;
    }
    if (value == -999) {
        left.value = 777;
        sys->sleep(60000);
        raise "killed callback resumed";
    }
    if (value == -2) {
        saved := left.value;
        left.value = -2222;
        while (left.value == -2222) {
            sys->sleep(1);
        }
        if (left.value != saved) {
            raise "delayed callback gate changed original value";
        }
        value = 1;
    }
    left.value += value;
    if (value == -1) {
        raise "partial callback effect";
    }
    right.value += value;
    return left.value;
}
