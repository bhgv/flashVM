# DisVM: выборка ядра Dis и JIT

[English](README.md) · [Manifest](UPSTREAM.json) ·
[Инструменты и обновление](../tools/UPSTREAMS.ru.md) · [Архитектура](ARCHITECTURE.ru.md)

Источник: https://github.com/bhgv/Inferno-OS-bhgv, активная ветка **experiment**.
Текущий снимок фиксирует 3e3536578b969cf1a460210d8ec667713869ccfc. Каждый файл имеет
SHA-256 в manifest. Сборки сами не подтягивают неизвестный HEAD; обновление явное.
Другой fork не подменяет это ядро.

```sh
./build-workspace.sh --projects disvm --tests
```

Либо независимо: `cmake -S disVM -B build/disvm`,
`cmake --build build/disvm --parallel 2`,
`ctest --test-dir build/disvm --output-on-failure`.

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

На Linux AMD64 пятнадцать тестов проверяют integer helpers, queues/transfers каналов,
Dis-parser/error cleanup, pointer maps, serialization/layout widths и настоящее
исполнение опубликованных AMD64 leaf-функций для WORD32/WORD64 fixture models.
Используются реальные mmap RW и RX publication. Другие VM-границы в fixtures
инъецированы: это не целый Limbo-процесс с scheduler/GC и не Dis/SWF-интеграция.
Физическое исполнение ARM64/RV64 не заявляется.

[Собственный Limbo — приоритет](../compiler/limbo/OWN_COMPILER.ru.md).
[Один .m для внешнего AS и AS-клиента Dis](../compiler/limbo/examples/external-modules/README.ru.md).
Временный native bootstrap вынесен в tools/limbo-bootstrap.

[Состав и проверки обновления 0.1.1](SYNC.ru.md)

[Необходимые службы встроенного хоста](HOST-REQUIREMENTS.ru.md)

[Общий ввод-вывод, TLS-регистры и четыре порта](host/README.ru.md) · [Пример передачи модуля](examples/module-image/README.ru.md)

Контрольная точка v0.1535: активный REG встроен в расширенный up, добавлены безопасные точки и остановка для обхода корней. Полные параллельные GC/VM ещё не завершены. [Контракт](host/README.ru.md).

[Контракт исполнителей GC](host/GC.ru.md) — v0.1536.
