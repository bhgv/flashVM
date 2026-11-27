# styxrpc_module_live_open

[English](module_live_open.md) · [Объявление](../include/styxrpc/module_client.h) ·
[Реализация](../src/live_module_client.c)

```c
#include <styx/client/client.h>
#include <styxrpc/module_client.h>

struct StyxFid *styxrpc_module_live_open(struct StyxClient *session,
    const char *path, struct StyxFid **lease);
```

Хостовый SDK запрашивает загрузку обычного файла `.dis` у подключённого DisVM.
`session` заимствуется. `path` — видимый канонический путь от корня сессии,
без начальной косой черты, до `STYXRPC_VFS_PATH_MAX` байт. DisVM разрешает его
во время загрузки. `lease` указывает на доступный для записи слот. При
успехе метод возвращает собственный неоткрытый fid каталога, а в `*lease`
выдаёт собственный fid запроса. При отказе возвращается NULL и `*lease`
сохраняется.

Функция опрашивает готовность до четырёх секунд с учётом сроков нативного
I/O сессии. Запрос выполняется один раз. Отсутствие службы, отказ загрузки
или публикации, а также сбой транспорта дают NULL. Вызывайте функцию из
потока, который может ждать Styx. У каждого вызова свой курсор запроса.

```c
struct StyxFid *lease = NULL;
struct StyxFid *module = styxrpc_module_live_open(session, "lib/counter.dis", &lease);
StyxRpcModuleExports exports;
if (module && styxrpc_module_exports_at(module, 0, &exports) == STYXRPC_OK) {
    /* Здесь можно вызывать проверенные экспорты через точный fid. */
}
styxclientclunk(module);
styxclientclunk(lease);
```

Удерживайте аренду на время всех вызовов и использования возвращённых
каналов. Закрытие отзывает загруженное поколение. См.
[обнаружение экспортов](module_exports_at.ru.md) и [точные Int32-вызовы](scalar_invoke_at.ru.md).
