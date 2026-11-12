# Copyright (c) 2026 bhgv
# EN: One resident for closed-admission error probes without unfinished producers.
# RU: Один процесс для проверки закрытого допуска без незавершённых производителей.
implement ConcurrentRetirement;
include "sys.m";
ConcurrentRetirement: module { init: fn(); };


# EN: Borrow Sys and invoke the real checkpoint; its host probe owns test objects.
# RU: Заимствовать Sys и вызвать настоящий checkpoint; объектами владеет host-тест.
init()
{
    sys := load Sys Sys->PATH;
    sys->werrstr("concurrent retirement checkpoint");
}
