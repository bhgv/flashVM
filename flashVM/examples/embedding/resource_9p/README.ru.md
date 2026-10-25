# Ресурс через 9P2000

Собираемый C89 пример читает `/1.1/path` через стандартные двоичные
version/attach/walk/open/read кадры. VM, SDL, сокетов и mount нет.

```sh
cmake -S flashVM -B build-9p -DFVM_BUILD_EXAMPLES=ON
cmake --build build-9p --target flashvm_examples_embedding_resource_9p_main
./build-9p/flashvm_examples_embedding_resource_9p_main
```

Внутри компонента: `cmake -S .`. Требуются C89 и CMake, линковка только
с flashvm_srecord. Ожидается `9p resource path: /scene/root`.
Исходник содержит читаемые подписанные wire fixtures; `1.1` верно только
для свежей регистрации. В приложении имена перечисляются. Хост выдаёт права
заранее; uname не выдаёт авторитет. FS заимствует диспетчер, сервер — FS,
сервер владеет fids. Очистка: server, FS, dispatcher. Ошибки тоже проходят
эту очистку; для BUSY production хост ждёт callback и повторяет destroy.
В этом примере callback не вызывается: читается metadata файла path.
Нет TCP listener, аутентификации, ОС mount и полного Inferno namespace.

| API | Владение / исполнение |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L109) | Создаёт принадлежащий хосту single-owner диспетчер. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_register`](../../../include/flashvm/rdispatch.h#L115) | Копирует описание ресурса, заимствует callback userdata. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L128) | Owner выдаёт READ доверенной локальной сессии. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rfs_create`](../../../include/flashvm/rfs.h#L55) | Создаёт FS с исключительными ID сессии, заимствует диспетчер. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_r9p_create`](../../../include/flashvm/r9p.h#L22) | Создаёт fixed соединение, заимствует FS. [doc/RESOURCE_9P.ru.md](../../../doc/RESOURCE_9P.ru.md). |
| [`fvm_r9p_request`](../../../include/flashvm/r9p.h#L32) | Обрабатывает кадр в выходной буфер без исполнения VM. [doc/RESOURCE_9P.ru.md](../../../doc/RESOURCE_9P.ru.md). |
| [`fvm_r9p_poll`](../../../include/flashvm/r9p.h#L40) | Проверяет ожидающие чтения; PENDING сохраняет выход/счётчик. [doc/RESOURCE_9P.ru.md](../../../doc/RESOURCE_9P.ru.md). |
| [`fvm_r9p_destroy`](../../../include/flashvm/r9p.h#L47) | Закрывает fids до удаления сервера; FS принадлежит хосту. [doc/RESOURCE_9P.ru.md](../../../doc/RESOURCE_9P.ru.md). |
| [`fvm_rfs_destroy`](../../../include/flashvm/rfs.h#L104) | Освобождает FS после закрытия сервером всех дескрипторов. [doc/RESOURCE_FILES.ru.md](../../../doc/RESOURCE_FILES.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L163) | Освобождает остановленный диспетчер после завершения заимствований. [doc/RESOURCE_DISPATCH.ru.md](../../../doc/RESOURCE_DISPATCH.ru.md). |

[main.c](main.c) · [catalog](../../README.md#example-embedding-resource_9p)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_r9p_create` | function | [include/flashvm/r9p.h:22](../../../include/flashvm/r9p.h#L22) |
| `fvm_r9p_destroy` | function | [include/flashvm/r9p.h:47](../../../include/flashvm/r9p.h#L47) |
| `fvm_r9p_poll` | function | [include/flashvm/r9p.h:40](../../../include/flashvm/r9p.h#L40) |
| `fvm_r9p_request` | function | [include/flashvm/r9p.h:32](../../../include/flashvm/r9p.h#L32) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:109](../../../include/flashvm/rdispatch.h#L109) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:163](../../../include/flashvm/rdispatch.h#L163) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:128](../../../include/flashvm/rdispatch.h#L128) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:55](../../../include/flashvm/rfs.h#L55) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:104](../../../include/flashvm/rfs.h#L104) |
<!-- source-links:end -->
