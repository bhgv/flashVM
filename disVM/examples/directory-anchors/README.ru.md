# Anchors каталогов у native owners

Исполняемый пример сохраняет cwd/import resources у четырёх системных потоков,
когда namespace меняется, а родитель удаляет свои ссылки. Применение — работающие
workers продолжают использовать cwd после изменения привязок приложения.
[English](README.md), [полный контракт](../../runtime/DIRECTORY-ANCHORS.ru.md).

`main.b` связывает source-a с visible, source-b с other. Children делают FORKNS,
chdir visible, открывают два независимых cursor. Родитель удаляет свои bindings
до продолжения children. Каждый child делает bind поверх cwd, проверяет чтение,
import/create, затем выходит из cwd и наблюдает новую virtual route. Unmount с
source filter сравнивает object identity. Старый provider работает после снятия
namespace refs.

Локальные FD — managed references, ненужные явно очищаются. Группы освобождаются
с owners; каналы передают только scalar ready/start/PID/results. Нет native
указателей между heaps или global stop. Обычный Rename=0 не требует controller.
Драйвер собирает второй вариант, ждёт readiness, реально переименовывает каталоги,
ставит подмены по старым именам и разрешает продолжение.

Команды подготовки каталогов, сборки двух providers и main, обычного запуска и
полной проверки приведены в [README.md](README.md). Ожидается:
`ANCHORSDONE workers=4 bad=0 cwd=A route=B module=11`.
Драйвер требует zero live anchors при shutdown и обнаруживает контрольное
обращение к подмене. Проверяются interpreter/JIT и оба GC polls. Python и compiler
нужны для разработки, не как установленные зависимости runtime.

| API | Назначение и владение |
| --- | --- |
| Sys.pctl / chdir | Копирование refs группы, удержание cwd; host cwd не меняется. |
| Sys.bind / unmount | Изменение MREPL route; открытые cwd/cursors сохраняются. |
| Sys.open / read | Локальный rooted FD удерживается при чтении child каталога. |
| Sys.create / write | Независимый output живёт до очистки; запись в исходный ресурс. |
| Sys.dirread | Независимый bounded batch каждого cursor и его metadata. |
| Processctl.pid / killwait | PID children и ожидание native retirement. |
| load | Удержание module instance и его исходного кода. |

[Sys namespace](../../runtime/VFS-NAMESPACE.ru.md),
[загрузка](../../runtime/MODULE-NAMESPACE.ru.md),
[source contracts](../../runtime/vfs/resource.h).
Проверен Linux x64. Полный mount-parent walk, identity отдельного файла при его
rename и non-Linux runtime adapters этим примером не заявляются.
