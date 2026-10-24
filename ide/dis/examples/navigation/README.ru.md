# Пример навигации с проверкой ревизии

[English](README.md) · [Сборка](../../README.ru.md) · [Протокол](../../PROTOCOL.ru.md)

[main.b](main.b) — полностью комментированный Limbo-клиент настоящего владельца
документа. Открывается `A中\nxyz\n`, смещение 4 преобразуется в строку 2/столбец 1
и обратно, находится `yz`, затем заменяется на `YZ` с проверкой найденной ревизии.
Последующий запрос со старой ревизией отклоняется. Прежний снимок текста сохраняется.
После правки saved=1/revision=2: документ dirty, но запись файла не имитируется.
Все операции проходят через типизированные request/reply channels.

## Сборка и исполнение

Настройте явные Limbo compiler, emulator и module headers по
[инструкции](../../README.ru.md). При BUILD_TESTING и EMU_EXECUTABLE CMake собирает
navigation.dis из этого исходника. CTest запускает `ide_dis_navigation_mode_0`
и `_1`. Из корня workspace:

```sh
# Собрать продукт и пример уже настроенными внешними инструментами.
cmake --build build/ide-dis-test
# Проверить успешный exit и обязательный маркер PASS.
ctest --test-dir build/ide-dis-test -R ide_dis_navigation --output-on-failure
```

Ожидаются `match offset=4 line=2 column=1` и
`PASS idecore navigation 16 checks`. Sys используется только bootstrap-примером
для отчёта и остановки своей отдельной reference VM. Ядро не импортирует Sys
или графику. Это developer bootstrap, не готовый UI. Клиент и модуль требуют
одинакового idecore.m: Reply ABI 0.3.x отличается от 0.2.x. Режимы reference VM
исполняют Dis, но не доказывают вход в native JIT.

Координаты измеряются символами Limbo, строка от 1, столбец от 0. Только LF
начинает строку; CR остаётся символом. UTF-8-byte диагностику компилятора и
UTF-16 offsets редактора нужно преобразовывать отдельно. Файловое сохранение,
парсер, UI-виджет и execution debugger этим примером не реализуются.
