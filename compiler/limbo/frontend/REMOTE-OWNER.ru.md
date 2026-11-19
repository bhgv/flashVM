# limbo_remote_owner

Copyright (c) 2026 bhgv

[English](REMOTE-OWNER.md) · [Руководство и real-VM test](../../../disVM/examples/remote-scalar/README.ru.md) ·
[Header](remote_owner.h) · [Реализация](remote_owner.c)

```c
#include "remote_owner.h"
int limbo_remote_owner(FILE *output, const LimboInterface *catalog,
                       const char *include_name, const char *implementation,
                       char *error, size_t capacity);
```

Сгенерировать owner-side Proxyhandler adapter для единственного экспорта
`fn(int):int` в catalog. C99 API принадлежит `limbo_remote_owner_lib`, линкующей
существующий `limbo_interfaces`/Coco/R frontend. Dis не исполняется, transport
не создаётся, C++ runtime не нужен.

| Параметр | Контракт |
| --- | --- |
| output | Borrowed stream; NULL выполняет полный preflight без output |
| catalog | Borrowed immutable parsed interface, живущий до возврата |
| include_name | Borrowed basename того же интерфейса, доступного Dis compiler |
| implementation | Borrowed provider-local path для generated ordinary typed load |
| error | Optional caller-owned diagnostic buffer; при capacity>0 очищается при входе |
| capacity | Размер diagnostic buffer с NUL; ноль отключает запись |

Возврат1 — supported/generated success; 0 — unsupported input, неверный path/module
name, semantic validation failure или stream failure. Полный profile validation
предшествует первому output byte. Stream failure может оставить prefix. Caller
владеет close, при необходимости atomic publication и catalog retirement.
Input pointers не сохраняются.

Supported profile, path bounds, unsupported operations и runnable example описаны
в руководстве. Immutable catalogs можно независимо читать; parsing сериализуется
по существующему frontend API, shared output/error buffers синхронизует caller.
Совместимость интерфейса не является execution permission/authentication.
Generated Dis load проверяет actual provider implementation; automatic client-side
module binding остаётся отдельным участком работы.
