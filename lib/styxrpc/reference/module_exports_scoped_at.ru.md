# styxrpc_module_exports_scoped_at

[English](module_exports_scoped_at.md) · [Объявление](../include/styxrpc/module_client.h) ·
[Реализация](../src/client_exports.inc)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_exports_scoped_at(struct StyxFid *directory,
    uint32_t pid, StyxRpcModuleExports *out, uint32_t *scoped);
```

Находит проверенные контракты на заимствованном неоткрытом `directory`
по правилам [styxrpc_module_exports_at](module_exports_at.ru.md).
Ненулевой `pid` допускает импорт методов, которым нужен контекст клиента.
Требования проверяются на том же каталоге, что имена и файлы методов.

При `STYXRPC_OK` поля и имена принадлежат `out`. Сохраняйте его по
исходному адресу. Если передан `scoped`, бит `i` сообщает, требуется ли
`out->methods[i]` встречный контекст клиента. Например, после успешного
вызова `scoped & UINT32_C(1)` проверяет первый экспорт. При
`STYXRPC_INVALID` оба выходных значения сохраняются.

Метод может ждать операций Styx. Вызывающий удерживает каталог и
закрывает его после завершения обнаружения экспортов.
