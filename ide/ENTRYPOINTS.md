# Запуск приложений / Application entry points

[Workspace gestures / Жесты и оформление](DESKTOP-WORKSPACE.md) · [Русский](DESKTOP-WORKSPACE.ru.md)

После сборки `disvm-editor` пункт **8** конечного build-menu запускает новый
MovieClip-стол. Пункт **9** сохраняет прежний одиночный редактор. Это разные Dis
приложения, исполняемые нашей встроенной VM. Раньше пункт 8 ссылался на старый
`run-ide.sh`, а desktop launcher не участвовал в меню — новые возможности поэтому
не показывались при привычном запуске.

| Пункт / Entry | Команда в установленном пакете / Installed command | Назначение / Purpose |
| --- | --- | --- |
| 8 / desktop | `sh run-desktop-ide.sh` | Парная IDE на Limbo и ActionScript. Два штатных процесса связаны локальным StyxRPC |
| 9 / editor | `sh run-ide.sh` | Прежний одиночный графический редактор с его отдельными интеграциями |
| 10 / lifecycle | `sh run-desktop-lifecycle.sh` | Переносимый пример независимых владельцев, завершения и нового документа |
| 11 / pointer | `sh run-desktop-pointer.sh` | Переносимый пример Unicode-выделения и отклонения stale revision |
| 17 / tokens | `sh run-token-query.sh` | Coco/R: контекст, поиск, лексические кандидаты; настоящий пример Limbo без GUI |
| 16 / syntax | `sh run-syntax-check.sh` | Короткий пример настоящих Coco/R-сканеров Limbo/AS, Unicode и отклонения устаревшей проекции; без GUI |
| 19 / resident-owners | `sh run-resident-owners.sh` | Четыре закреплённых нативных потока, личное состояние и ответы через каналы; без пула и миграций |
| 12 / clipboard | `sh run-desktop-clipboard.sh` | Сессионный Copy/Cut/Paste через отдельного владельца Limbo |

Пункты 10/11/12 доступны также в `disvm-runtime`, без графики. Они запускают настоящие
Dis-модули и завершаются после PASS; это не макеты GUI. Оконные launcher-скрипты
поставляются в пакете `disvm-editor`. Пункт 8 доступен в графической сборке Linux64
с SDL2. Сборка `--no-player` оставляет `run-desktop-local.sh` для прежнего replay.

Контроллер из пункта 8 также собирается для обычного DisVM. Статус
двухпроцессного запуска и проверка правки с сохранением описаны отдельно.
[Парный desktop](DESKTOP-PAIRED.ru.md) · [English](DESKTOP-PAIRED.md).

Из исходного дерева можно собрать и сразу запустить новый стол без отдельного
интерактивного выбора:

```sh
python3 build-menu.py --projects disvm-editor --backend cmake --launch desktop
python3 build-menu.py --projects disvm-editor --backend cmake --launch desktop --document "my source.b"
python3 build-menu.py --projects disvm-runtime --backend cmake --launch pointer
```

Для предыдущего редактора используйте `--launch editor`, для примера завершения —
`--launch lifecycle`, для сессионного буфера — `--launch clipboard`. `--document` относится только к desktop/editor. Путь передаётся
отдельным argv, не исполняется shell. `--launch` требует установки и не сочетается
с `--no-install`. В режиме `--projects` без `--launch` итоговый список всё равно
показывает установленные launcher-скрипты. Отсутствующий общий `run-checks.sh` больше
не скрывает самостоятельные приложения.

Шрифт Noto Sans Mono/OFL поставляется с пакетом. Парная IDE загружает его через VFS.
Пункт 9 поддерживает необязательный `FVM_EDITOR_FONT`. Нативные
скрипты запуска и приложения не требуют Python, внешнего emu или CLI-компилятора.
Python/Bash/CMake — инструменты сборки из исходников.

[Рабочий стол](DESKTOP.ru.md) · [Выделение](DESKTOP-POINTER.ru.md) ·
[Закрытие окон](DESKTOP-LIFECYCLE.ru.md) · [English desktop guide](DESKTOP.md)

## English

Post-build menu **8** launches the paired Limbo and ActionScript desktop with
the ordinary DisVM and FlashVM player executables on Linux64 with SDL2.
**9** keeps the older single-editor application. The paired desktop and
portable lifecycle/pointer/clipboard clients are now listed in CLI and interactive results.
A missing general checker no longer suppresses installed application launches.
Use `--launch desktop|editor|lifecycle|pointer|clipboard` after a successful source build;
`--document` is a literal path for the two graphical entries. Source builds need
CMake for these components. A `--no-player` build keeps `run-desktop-local.sh`
for embedded-renderer replay.

The desktop still has bounded visible text and drag selection within the body.
System clipboard, off-body capture/autoscroll, session persistence and attached Run/Debug
are separate integrations. The single-editor's extra capabilities are available
through entry 9; they are not silently claimed to be embedded in entry 8.

[Сессионный clipboard](DESKTOP-CLIPBOARD.ru.md) · [Session clipboard](DESKTOP-CLIPBOARD.md).

[Lazy syntax service / Ленивый анализ Coco/R](SYNTAX-LAZY.md) · [Русский](SYNTAX-LAZY.ru.md).

Menu8 presents lazy Limbo/AS colours through the native contour extension; menu16 checks the service alone. [Contract](SYNTAX-LAZY.md).

[Context/search/candidates](TOKEN-QUERIES.md) · [Контекст/поиск/кандидаты](TOKEN-QUERIES.ru.md).


Menu **8 → Draw** / **Ctrl+Shift+N** creates an independent vector window. B/R/E/L, colours1..6, F fill, Ctrl+Z/Y, Ctrl+S/O SVG. A `.svg` launch path selects a vector owner. [Guide EN](VECTOR-EDITOR.md) · [RU](VECTOR-EDITOR.ru.md).

[Пример закреплённых владельцев — EN](../disVM/examples/resident-owners/README.md) · [RU](../disVM/examples/resident-owners/README.ru.md)
