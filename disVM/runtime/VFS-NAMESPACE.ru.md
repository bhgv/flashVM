# Native-группы namespace: первый блок VFS

Каждый resident владеет native-ссылкой на группу namespace. `spawn` разделяет
группу родителя. В ней хранятся virtual cwd, native root и заменяющие привязки.
Группа имеет собственный переносимый mutex; глобальная exclusive-секция либо
остановка VM не добавляются. В native-памяти группы нет managed pointers/GC roots.
Отказ подготовки process и окончательный retirement освобождают ссылку; потомки
могут пережить родителя. Это рабочее состояние namespace, а не постоянно
поддерживаемый каталог объектов для статистики.

## Стандартные Sys-вызовы

| Вызов | Реализованное поведение |
| --- | --- |
| `pctl(0, nil)` | Вернуть текущий PID |
| `pctl(FORKNS, nil)` | Отделить согласованную копию root, cwd и bindings |
| `pctl(NEWNS, nil)` | Отделить пустой namespace с root в разрешённом текущем каталоге; virtual cwd становится `/` |
| `chdir(path)` | Проверить каталог и изменить cwd группы; host process cwd не меняется |
| `bind(source, on, MREPL)` | Заменить существующий однотипный ресурс в точном virtual target |
| `unmount(source, on)` | Удалить замену только при совпадении сохранённого native source |
| `unmount(nil, on)` / пустой source | Удалить точную замену без фильтра source |

Другие `pctl` flags, сочетания flags, non-nil `movefd`, union/create/cache flags
для bind и отсутствующие привязки дают `-1` с guest error. Успех не имитируется,
частичного изменения namespace нет. Ограничения create прежние: `OWRITE`,
опциональный `OEXCL`, permission `0666` с учётом host umask.

`Sys.open`, `Sys.create` и `Sys.stat` разрешают namespace вызывающего. Read,
write, seek, fstat и dirread работают через существующие handles; новая привязка
не перенаправляет ранее открытый FD. Побеждает длиннейший prefix на границе
компонента; `/view` не совпадает с `/view-other`. Source разрешается один раз
при bind; последующее переподключение source не создаёт alias-рекурсию.
Более конкретные привязки сохраняются при замене/удалении родительского prefix.

## Конкурентность и владение

План пути копирует virtual/native names и версию группы под её mutex. Host I/O
использует только native-копию после unlock и выхода из managed admission через
имеющийся native I/O adapter. Конкурентное изменение namespace не перенаправляет
уже начатый open/stat. Вне managed admission указатель группы не используется.

Для chdir/bind проверочный I/O выполняется вне mutex. Commit допустим только
при совпадении наблюдавшейся версии. Конфликт повторяется до восьми раз, затем
возвращается явная ошибка. Версия никогда не переполняется; исчерпание запрещает
изменения. FORKNS копирует группу согласованно под её mutex. NEWNS берёт
согласованный снимок resolved cwd; дальнейшие изменения соседей относятся
только к исходной группе. Привязки/копии/ссылки используют native allocation
и не вызывают VM с удерживаемым namespace mutex.
Scalar return публикуется через существующие shared-cell guards. При BUSY
повторяется только publication; уже применённое изменение namespace не повторяется.

## Точная граница этого блока

Это **маршрутизация host-путей**, ещё не полная Inferno Chan/provider VFS и не
security sandbox. Native path strings не сохраняют identity ресурса при host
rename/unlink. Host продолжает следовать symlinks. Virtual `.`/`..` нормализуются
лексически до поиска bind; это не обход host symlinks. Максимум —4096 UTF-8 bytes
с NUL. Пустые/слишком длинные пути отклоняются. Host ФС может измениться между
проверкой и дальнейшим доступом.
Здесь cwd — virtual path, а не открытый Chan: замена охватывающего cwd prefix
влияет на последующий relative lookup. Стабильная identity каталога при
rebind/rename требует следующего слоя provider handles.

Module loader теперь использует frozen namespace plans для cache/open/parse:
см. [module namespace и cache provenance](MODULE-NAMESPACE.ru.md). Начальные
relative imports идут от entry package; chdir обновляет это состояние группы.
Стандартные FD и явно включённый test-control node также сохраняют прежние
маршруты. FORKNS/NEWNS не изолируют FD groups, environment variables либо
credentials. Существующий FD может вести за пределы нового root.

Union mounts, mount transport, device/socket/channel providers, provider handles,
API исследования namespace, FD/env/credential groups и dynamic C/Limbo provider
loading остаются отдельной работой. Семантика shared-cwd/FORKNS и NEWNS-rebase
сверена с `emu/port/inferno.c` ветки Inferno experiment. Vendor source не менялся,
старые OS exclusive-механизмы не импортировались. Контракты RU/EN находятся в `vfs/`.

| Платформа | Что подтверждает этот блок |
| --- | --- |
| Linux x64 | Реальная VM: interpreter/JIT, оба GC poll, обычная и TSan-сборки |
| Linux ARM64 / RV64 | Добавлен обязательный workload `namespace`; требуется архитектурная CI |
| Windows / macOS / Android | Общее ядро использует libc и существующий portable mutex API; исполняемый runtime для этих ОС этим изменением не перенесён и не проверен |

Runtime остаётся самодостаточным; Python/CMake — средства разработки, не
установленные зависимости для исполнения. [Пример](../examples/namespace/README.ru.md),
[English](VFS-NAMESPACE.md).

Проверки: полный обычный набор67 групп прошёл; целевые11 прошли обычно и под
TSan, ещё12 TSan-групп проверили lifecycle/debugger. После финальной guarded-result
правки namespace/concurrency повторены успешно в обеих сборках. Четыре writers
пишут return прямо в shared field в обоих execution/poll. Тестовый пропуск
guards даёт настоящую TSan data race в interpreter/JIT; normal exit/crash/compiler
failure не засчитываются. Отдельное ядро прошло ASan/UBSan с отключённым leak
detection из-за sandbox ptrace; leak coverage этим запуском не заявляется.
