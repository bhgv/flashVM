# DisVM: выборка ядра Dis и JIT

[English](README.md) · [Manifest](UPSTREAM.json) ·
[Инструменты и обновление](../tools/UPSTREAMS.ru.md) · [Архитектура](ARCHITECTURE.ru.md)

Источник: https://github.com/bhgv/Inferno-OS-bhgv, активная ветка **experiment**.
Текущий снимок фиксирует 51227cb0629bc6fd755d4d6ecad44f055ea56690. Каждый файл имеет
SHA-256 в manifest. Сборки сами не подтягивают неизвестный HEAD; обновление явное.
Другой fork не подменяет это ядро.

Серия v0.1549: [исполняемые выбранные модули](libraries/README.md),
[подробно комментированный пример](examples/standard-modules/README.md),
[рантайм](runtime/README.md). Пункт 7 меню запускает реальные модули;
готовность функций и ещё не подключённые службы /prog/VFS указаны отдельно.

```sh
./build-workspace.sh --projects disvm --tests
```

Либо независимо, в том числе с CMake 3.10:

```sh
cmake -HdisVM -Bbuild/disvm
cmake --build build/disvm -- -j2
(cd build/disvm && ctest --output-on-failure)
```

Собирается **libdisvm_core.a** — настоящие upstream interpreter-объекты и JIT
по host CPU: amd64/arm64/riscv64. Для архива нужны POSIX C и CMake; тестам —
Python 3 и C-компилятор. Limbo/CocoR, SDL/Tk, графический SDK, crypto-сервис и
полная Inferno не нужны. Старый 386 backend оставлен как reference.

## Что ещё не готово

Архив **пока не самостоятельная исполняемая Dis VM**. Нужны настоящие host/kernel
функции kernel.h, scheduler/service integration, регистрация модулей и I/O.
Они не заменены заглушками «успех». Архив не запускает idecore.dis и не является
EMU_EXECUTABLE. Native builtin frame/layout migration и совместимость ABI
компилятора/runtime остаются отдельной работой upstream/порта.

Новые 64-битные JIT — decoded-operation leaf JIT: Dis PC сохраняются,
Module.compiled остаётся 0, используется Module.jit. Часть операций нативна;
аллокации, ownership, вызовы, исключения и планирование сохраняют interpreter/
host-пути. Наличие исходника не означает приёмку всех операций и платформ.
Новый общий Dis64 ABI и объединённый контейнер SWF/Dis здесь не объявляются.

## Что перенесено

Ядро/декодер, heap/GC/stack/string/channels/loader/Sys, нужные pointer-map/JIT
заголовки, AMD64/AArch64/RV64 emitters, 386 reference, интерфейсы/NOTICE и
необходимые regression fixtures. Малые emu/os boundary-файлы нужны upstream
тестам, которые извлекают настоящие тела функций; полная ОС не собирается.
GUI/security реализации, шрифты, приложения, архивы и платформенные деревья
не переносятся. В generated runt.h сохранены некоторые builtin declarations;
это не линковка соответствующих графических/security-модулей.

POSIX declaration adapter лежит отдельно, в host/include. ARM-only inline
return-address не исполняется на native host. Это не полный scheduler/FPU-port.
Upstream blobs неизменны, их лицензии сохранены.

## Проверки

На Linux AMD64 регрессии проверяют integer helpers, queues/transfers каналов,
Dis-parser/error cleanup, pointer maps, serialization/layout widths и настоящее
исполнение опубликованных AMD64 leaf-функций для WORD32/WORD64 fixture models.
Используются реальные mmap RW и RX publication. Другие VM-границы в fixtures
инъецированы: это не целый Limbo-процесс с scheduler/GC и не Dis/SWF-интеграция.
При включённых native registers текущий набор содержит 39 тестов, включая
host/GC и покрытие эмиттеров. Физическое исполнение ARM64/RV64 не заявляется.

[Собственный Limbo — приоритет](../compiler/limbo/OWN_COMPILER.ru.md).
[Один .m для внешнего AS и AS-клиента Dis](../compiler/limbo/examples/external-modules/README.ru.md).
Временный native bootstrap вынесен в tools/limbo-bootstrap.

[Состав и проверки обновления 0.1.1](SYNC.ru.md)

[Необходимые службы встроенного хоста](HOST-REQUIREMENTS.ru.md)

[Общий ввод-вывод, TLS-регистры и четыре порта](host/README.ru.md) · [Пример передачи модуля](examples/module-image/README.ru.md)

Контрольная точка v0.1535: активный REG встроен в расширенный up, добавлены безопасные точки и остановка для обхода корней. Полные параллельные GC/VM ещё не завершены. [Контракт](host/README.ru.md).

[Контракт исполнителей GC](host/GC.ru.md) — v0.1536.

[Обновление experiment и готовность к IDE / experiment update and IDE readiness](UPSTREAM-UPDATE.md).

[Value/channel UI bridge](UI-BRIDGE.md) · [Канальный мост и ввод FlashVM](UI-BRIDGE.ru.md).

[Owner pump / Прокачка и сохранение ввода](OWNER-PUMP.ru.md).

[Versioned module namespace / Версии модулей](MODULE-STORE.md).


## Runnable native owner / Исполняемый нативный рантайм

The separate [experimental NC64 runner](runtime/README.md) now loads and executes
matching Limbo modules with production interpreter/JIT, blocking channels and IDE
services. Select `disvm-runtime` or `all` in `build_menu.py`; post-build menu items
4/5 launch Dis and IDE console examples. The core archive above remains a separate
classic SDK. Full GUI, Sys/VFS coverage and parallel shared VM owners remain open.

[Графический редактор v0.1550](../ide/dis/examples/graphical-editor/README.ru.md):
отдельный исполняемый target `disvm-editor` соединяет Dis-контроллер с TextField
FlashVM. Описание незавершённого хоста выше относится к отдельному classic SDK,
а не к этому экспериментальному NC64 runtime.

## Обновление experiment v0.1555

Снимок fa3b7abf добавляет guarded JIT десятичных преобразований и записи символов
для AMD64/ARM64, проверки backing spans и логической ёмкости строк. В текущей
выборке 66 обычных проверок ядра и две необязательные проверки инструкций через
capstone/unicorn. Для них задаётся `DVM_EMULATOR_PYTHONPATH` либо зависимости
доступны выбранному Python. Это только тестовые инструменты, не зависимость VM;
физическое исполнение других CPU и полный ABI-порт отдельно не заявляются.
[Проверки серии](../docs/releases/v0.1555.md).


## Проверки чтения строк v0.1558

Текущая публичная ревизия: `61a2942bbe51dd3af507862d7c7081ff5ed8e6cf`:
проверки ёмкости при чтении, срезах, копировании, конкатенации и UTF-экспорте из
e6c056c и отправленное обратно исправление теста реального C/JIT-диспетчера.
В выборке 66 обычных и три необязательных проверки инструкций — все проходят.
Матрица чтения исполняет инструкции AMD64/ARM64 через эмуляцию; физическое
исполнение другой архитектуры этим не объявляется. [Проверки](../docs/releases/v0.1558.md).


## Real и дописывание строк v0.1561

Текущая публичная ревизия: `ddbc9f5d6412fe1d4630d7cba00f13502c9b041d`.
Прошли все 72 проверки выбранного ядра, включая native и инструкции AMD64/ARM64
для сравнений строк. [Комментированный исполняемый пример](examples/decimal-append/README.ru.md)
показывает real, знак нуля и сохранение владельцев Unicode-строк на настоящем
Dis. [Проверки серии](../docs/releases/v0.1561.md).


## ASCII, расширенные числа и буфер v0.1564

Текущая ревизия — `1bda41a0e140dabdb76a179c867315b0421df280`. Прошли все 75
проверок выбранного ядра, включая сохранённый парсер libmath. Редактор использует
[приватную сборку буфера на Dis](../ide/dis/CLIPBOARD.ru.md): значения, диапазон
и одна операция Undo принадлежат Dis. [Серия](../docs/releases/v0.1564.md).


## Rune-конкатенация и удержание Type, v0.1565

Текущая ревизия — `5ca377017fe42eac2d95ed0638a5b42ab45ac212`: Rune ADDC,
предварительные проверки удержания Type в heap/массивах/loader и upstream-тесты.
В выборке 76 проходящих проверок; runtime/editor и UBSan повторно проверяются
на этом pin. Тест memory-parser теперь переименовывает сохранённый entry
независимо от пробелов/переносов C. [Проверки](../docs/releases/v0.1565.md).

[Hosted hybrid big tables / Гибридные таблицы big](examples/hybrid-table/README.md): scalar C core and real Dis interpreter/JIT adapter; language lowering remains pending.
