# Запуск приложений / Application entry points

[Workspace gestures / Жесты и оформление](DESKTOP-WORKSPACE.md) · [Русский](DESKTOP-WORKSPACE.ru.md)

После сборки `disvm-editor` пункт **8** конечного build-menu запускает новый
MovieClip-стол. Пункт **9** сохраняет прежний одиночный редактор. Это разные Dis
приложения, исполняемые нашей встроенной VM. Раньше пункт 8 ссылался на старый
`run-ide.sh`, а desktop launcher не участвовал в меню — новые возможности поэтому
не показывались при привычном запуске.

| Пункт / Entry | Команда в установленном пакете / Installed command | Назначение / Purpose |
| --- | --- | --- |
| 8 / desktop | `sh run-desktop-ide.sh` | Несколько MovieClip-окон, миниатюры, перемещение, мышиная каретка/выделение, F7/F8, Help |
| 9 / editor | `sh run-ide.sh` | Прежний одиночный графический редактор с его отдельными интеграциями |
| 10 / lifecycle | `sh run-desktop-lifecycle.sh` | Переносимый пример независимых владельцев, завершения и нового документа |
| 11 / pointer | `sh run-desktop-pointer.sh` | Переносимый пример Unicode-выделения и отклонения stale revision |
| 12 / clipboard | `sh run-desktop-clipboard.sh` | Сессионный Copy/Cut/Paste через отдельного владельца Limbo |

Пункты 10/11/12 доступны также в `disvm-runtime`, без графики. Они запускают настоящие
Dis-модули и завершаются после PASS; это не макеты GUI. Оконные launcher-скрипты
имеются только в `disvm-editor`. Для физического окна требуется сборка с SDL2.
Явный `--no-player` даёт headless-вариант для replay, а не окно рабочего стола.

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

Шрифт Noto Sans Mono/OFL, размер 11, поставляется с пакетом. Искать его вручную не
нужно; прежний явный `FVM_EDITOR_FONT` остаётся необязательным override. Нативные
скрипты запуска и приложения не требуют Python, внешнего emu или CLI-компилятора.
Python/Bash/CMake — инструменты сборки из исходников.

[Рабочий стол](DESKTOP.ru.md) · [Выделение](DESKTOP-POINTER.ru.md) ·
[Закрытие окон](DESKTOP-LIFECYCLE.ru.md) · [English desktop guide](DESKTOP.md)

## English

Post-build menu **8** now launches the actual multi-window MovieClip desktop;
**9** keeps the separate older single-editor application. The old menu routed 8 to
`run-ide.sh` and never discovered `run-desktop-ide.sh`. The standalone desktop and
portable lifecycle/pointer/clipboard clients are now listed in CLI and interactive results.
A missing general checker no longer suppresses installed application launches.
Use `--launch desktop|editor|lifecycle|pointer|clipboard` after a successful source build;
`--document` is a literal path for the two graphical entries. Source builds need
CMake for these components. Physical windows need SDL2; `--no-player` is headless.

The desktop still has bounded visible text and drag selection within the body.
System clipboard, off-body capture/autoscroll, session persistence and attached Run/Debug
are separate integrations. The single-editor's extra capabilities are available
through entry 9; they are not silently claimed to be embedded in entry 8.

[Сессионный clipboard](DESKTOP-CLIPBOARD.ru.md) · [Session clipboard](DESKTOP-CLIPBOARD.md).
