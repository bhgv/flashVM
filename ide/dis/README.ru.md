# Переносимое ядро документа IDE

[Кнопки окна и скрываемые номера строк](WINDOWS-GUTTER.ru.md)

[Живой инспектор документа](LIVE-PROJECTIONS.ru.md)

[Значения, ссылки и отражение типов Limbo](VALUES.ru.md)

[Канальные проекции и модель инспектора](PROJECTIONS.md)

Версия 0.11.0. [English](README.md).

**Реализован** Limbo-модуль `idecore.dis` с `idecore.sbl`: владелец состояния одного
документа, управляемый типизированными каналами. Это первая исполняемая часть IDE,
не полный редактор и не приложение внутри FlashVM. Виджеты, файловое сохранение,
сервисы компиляции/отладки, встраивание Dis и мост AS здесь ещё не реализованы.
Продуктовый модуль включает только собственный интерфейс: без Sys, Draw, Tk,
Prefab, crypto и прямого native UI API.

Контракт — [idecore.m](idecore.m), реализация — [idecore.b](idecore.b), функции
`start`, `snapshot`, `handle`, `serve`. [Протокол](PROTOCOL.ru.md) описывает порядок,
ревизии, владение и остановку. Python используется только как инструмент тестов
на машине разработчика, не как код поставляемой IDE.

## Сборка исполняемого модуля

Нужен явно выбранный доверенный native Limbo-компилятор для машины сборки.
CMake не скачивает инструменты и не импортирует внешний runtime.

```sh
cmake -S ide/dis -B build/ide-dis \
  -DLIMBO_EXECUTABLE=/host/bin/limbo -DBUILD_TESTING=OFF
cmake --build build/ide-dis
cmake --install build/ide-dis --prefix release
```

Устанавливаются только `share/flashvm/ide/dis/{idecore.dis,idecore.sbl,idecore.m}`.
Тесты и вариант с уменьшенной границей ревизий не устанавливаются. `start()` —
библиотечная точка входа, не командный `init`. Будущий хост/клиент загружает модуль
и вызывает start; относительный PATH разрешается в его пространстве ресурсов.
Для исполнения модуля совместимой Dis VM Python не нужен.

Перед компиляцией старые результаты удаляются. Требуются успешный выход,
непустые module/debug файлы, unsigned Dis XMAGIC (`c0 0c 80 30`) и точные
первые две физические строки символов `limbo .sbl 2.1` / `Idecore`. Каждая
строка завершается LF или CRLF; пропуск пустых строк/NUL недопустим. Проверка
сигнатур читает только четыре байта модуля и не более 25 байт символов.
Это допуск продуктов сборки, не полноценный Dis verifier и не атомарная публикация.
При ошибке оба выхода удаляются; прежний опубликованный релиз надо хранить отдельно
от приватного каталога сборки.

## Исполнение проверок

```sh
cmake -S ide/dis -B build/ide-dis-test \
  -DLIMBO_EXECUTABLE=/host/bin/limbo \
  -DEMU_EXECUTABLE=/host/emu \
  -DLIMBO_MODULE_DIR=/host/inferno/module
cmake --build build/ide-dis-test
ctest --test-dir build/ide-dis-test --output-on-failure
```

Явно выбранный эмулятор запускается напрямую с тестовым init-модулем и приватным
корнем сборки, без shell и графического контекста. Только тесты загружают Sys и
останавливают собственный тестовый эмулятор через `/dev/sysctl`; продуктовый модуль
этого не делает. Нужны и PASS-маркер, и успешное завершение процесса; есть таймаут.
Проверяются режимы `-c0` и `-c1`. Они сами по себе не доказывают вход в native JIT.

`idecore_test.b` проверяет сеансы, правки, чистые/грязные ревизии, Unicode,
недопустимые и устаревшие команды. `limits_test.b` — точную границу 1048576 символов,
превышение, копирование запросов, сохранённые ответы, противодавление и конкурирующих
писателей. `revision_test.b` компилирует неизменённый продуктовый исходник с тестовым
MaxRevision=4. Вариант не устанавливается и не распространяется как idecore.dis.
`build_guard.py` проверяет 46 случаев допуска результатов компилятора.
[Руководство и ограничения](PRODUCT_ADMISSION.ru.md) содержит отдельную host-проверку.

Bootstrap-инструменты собраны из
[InferNode abb1b686](https://github.com/infernode-os/infernode/tree/abb1b6860238f89eb5a10125e561eff4e0a34836)
на Linux AMD64. Это внешний инструмент разработки/проверок, а не импортируемый
runtime FlashVM. Выбранная пользователем ветка Dis experiment сверена на
[1d8a3e3d](https://github.com/bhgv/Inferno-OS-bhgv/commit/1d8a3e3d3c9694385786b1df784bbd77c7ef9a75).
Исполнение в ней, интеграция FlashVM и native JIT x86-64/arm64/riscv64 требуют
отдельных проверок. Полные Coco/R-компиляторы Limbo ещё разрабатываются;
эта часть сейчас собирается явно выбранным bootstrap-компилятором.

## Формат поставки

IDE должна поставляться в интерпретируемом/JIT-формате: сначала Dis для Limbo,
с возможностью SWF для AS/FlashVM-частей. Python-утилиты и native-компиляторы —
инструменты сборки, а не само приложение IDE. Пока модули поставляются отдельно.
Общий контейнер SWF/Dis, дополнительные SWF-теги и общий загрузчик отложены.
Новый тег или формат объединения эта серия не вводит.

`.sbl` создаётся той же сборкой, что и модуль. Существующий C-читатель префикса
символов принимает 230 отображений логических PC этой сборки. Это не проверка
типов/локальных переменных и не управление отладчиком. Имена исходников могут
содержать пути машины сборки; будущему хосту понадобится отображение путей.
Исходники и символы следует держать вместе. [Проверки](VALIDATION.md).

## Навигация по исходнику

Версия 0.3.1 добавляет Locate/Offset через тот же типизированный канал с проверкой
ревизии. Пересоберите всех клиентов: Reply дополнен offset/line/column.
[Контракт координат](PROTOCOL.ru.md). Тест в обоих режимах проверяет все границы
пустых/Unicode/LF/CRLF текстов и ошибки stale/closed/range.

[Runnable navigation/search example / Пример навигации и поиска](examples/navigation/README.md).

[0.3.1 compiler/IDE acceptance / Приёмка серии](SERIES_0_3_1.md).

[Undo/Redo 0.5.0](HISTORY.md) · [Русский](HISTORY.ru.md) ·
[History example / Пример истории](examples/history/README.md).

[0.5.0 acceptance / Приёмка: новые проверки и открытые сбои VM](SERIES_0_4_0.md).

[Bulk replacement / Массовая замена](REPLACE_ALL.ru.md) · [Example / Пример](examples/replaceall/README.ru.md)

[Compiler UTF-8 source spans / UTF-8 диапазоны компилятора](UTF8_SPANS.ru.md) · [End-to-end example / Сквозной пример](examples/compiler-location/README.ru.md)

[Editor services / Сервисы редактора](../EDITOR-CHANNELS.ru.md).

[Dis workspace and compiler-job ownership / Рабочие документы и задания сборки](../WORKSPACE.md).

[Graphical editor / Графический редактор](examples/graphical-editor/README.md) ·
[Русское руководство](examples/graphical-editor/README.ru.md): executable Limbo
controller, actual Dis channels and FlashVM TextField presentation; build
`disvm-editor`, launch 8.

## Editor series v0.1551–v0.1554 / Серия редактора

[Navigation](NAVIGATION.md) · [Viewport](VIEWPORT.md) ·
[Actions](ACTIONS.md) · [Noto/zoom](ZOOM.md).
The current graphical package includes default Noto Sans Mono/11 without a font
question, Dis-owned scrolling/mouse/search/file actions and Ctrl-wheel/pinch zoom.

[Source editing / редактирование](EDITING.ru.md).

[Caret geometry / геометрия каретки](CARET.ru.md).

[Canvas resize / размер окна](RESIZE.ru.md).

[Clipboard / буфер обмена](CLIPBOARD.ru.md).

[Поля графической IDE](FIELDS.ru.md) имеют отдельную политику Idefield; исходное ядро документа 0.5.0 описано выше.

[Directory owner and console browser](DIRECTORIES.md) / [Каталоги через каналы](DIRECTORIES.ru.md).
