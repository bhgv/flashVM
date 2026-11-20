# styxorderedlaunchfileswithfds

[English](LAUNCH-FDS.md) · [Заголовок](launch.h) · [Реализация](launch.c) · [CLI](../../../../runtime/TERMINAL-LAUNCH-CLI.md)

```c
#include "launch.h"

StyxOrderedFiles *styxorderedlaunchfileswithfds(StyxClient *client,
    DVM_LaunchHandoff *handoff, const StyxOrderedOptions *options,
    const StyxLaunchFile *mappings, size_t count,
    const StyxLaunchFd *fds, size_t fd_count);
```

Функция native POSIX SDK подключает ordered-брокер и готовит частное
представление VFS терминала до запуска DisVM. При линковке нужна библиотека
`styx_ordered_files`. Функция заимствует принятый клиент терминала, эксклюзивный
подготовленный handoff, настройки брокера, оба массива и их строки до
возврата. `mappings` и `count` задаются по правилам
[`styxorderedlaunchfiles`](launch.h). В каждом элементе `fds` указываются
абсолютный путь у гостя, каноническое относительное имя файла в закреплённом
корне терминала и непустое сочетание `FVM_ACCESS_READ` и `FVM_ACCESS_WRITE`.
Счётчики задаются в элементах. Без выдачи файлов передайте `NULL, 0`.
Максимум — `STYX_LAUNCH_FDS_MAX` файлов.

На этапе подготовки каждый источник должен разрешиться в файл. Путь у гостя
не может пересекаться с другой выдачей, а источник должен отличаться от
выбранных узлов Unibus. Функция закрепляет файл через fid без открытия.
Открытие и операции ограничены заданными правами и политикой провайдера.
При [одноразовой публикации](../../../vfs/HELD-EXPORT.ru.md) первое успешное
открытие получает исходный файл с текущей позицией. Например, задайте
`{"/input", "held-input", FVM_ACCESS_READ}` и вызовите у гостя
`sys->open("/input", Sys->OREAD)`.

При успехе возвращается file-set во владение хоста, а слой терминала в handoff
заменяется. Хост вызывает `styxorderedfilesfree` после освобождения дескрипторов
и пространств имён гостя. При `NULL` исходный handoff остаётся доступным.
Отказ возможен при неверном пути или правах, отсутствующем файле, при попытке
выдать каталог, при отмене или сбое подготовки. Получение fid может ждать
Styx. Вызывайте функцию до передачи в runtime и вне блокировок VM и namespace.
