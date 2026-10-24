# Временный native bootstrap Limbo из исходников

[English](README.md) · [Приоритет собственного компилятора](../../compiler/limbo/OWN_COMPILER.ru.md) ·
[Обновление](../UPSTREAMS.ru.md)

Источник: https://github.com/bhgv/Inferno-OS-bhgv, ветка experiment. Commit и
SHA-256 — в UPSTREAM.json. Перенесены C-версия limbo/ и Limbo-версия
appl/cmd/limbo/, в том числе sbl.b/stubs.b как база для нашей разработки.
Это не выдаётся за наш Coco/R-компилятор: приоритет остаётся у compiler/limbo + FIR.

Bootstrap нужен потому, что числовой Limbo→FIR пока не собирает весь код IDE и
Coco/R-генератора. Теперь этот пробел закрывается собственной source-сборкой,
без готового чужого SDK. Native tool — временный инструмент сборки, не runtime
IDE. Исторический generated yacc-парсер сохранён как bootstrap-исходник;
устанавливать yacc не нужно. Наши новые парсеры остаются Coco/R.

```sh
./build-workspace.sh --projects limbo-bootstrap --tests
./build-workspace.sh --projects ide
```

Выборка включает lib9/UTF/форматирование, buffered I/O, преобразование чисел,
нужные .m и два MD5-файла для fingerprint типов Limbo. Сохранён исходный notice
RSA Data Security, Inc. MD5 Message-Digest Algorithm. Внешнего crypto SDK,
Keyring-сервиса, реализации GUI, полной ОС и готовых бинарников нет.
POSIX/fenv-адаптеры отдельны от upstream. ARM-only inline возвратного адреса
заменён на границе include compiler intrinsic; ARM-ассемблер не собирается.
Аппаратные FP traps bootstrap маскирует. Это не готовый host-порт всей VM/FPU.

## Debug и native-заготовки

Тесты проверяют unsigned Dis XMAGIC, -g/.sbl 2.1 и старый генератор C через -a.
В legacy-компиляторе -a даёт объявления, -t MODULE — runtime tables; исходники
сохранены для изучения и переноса возможностей в наш компилятор.

```sh
mkdir -p build/output
build/workspace/limbo-bootstrap/limbo-bootstrap -g -I ide/dis -o build/output/idecore.dis ide/dis/idecore.b
build/workspace/limbo-bootstrap/limbo-bootstrap -a ide/dis/idecore.m > build/output/legacy-ide-declarations.h
```

Сборка bytecode и проверка source map не означают приёмку исполнения VM.
Legacy compiler сохраняет исходные 32-битные соглашения данных/фреймов Dis;
новые 64-битные leaf JIT сами по себе не завершают native frame/layout port.
Компилятор и эмулятор из разных forks нельзя автоматически считать совместимыми.
Для IDE --tests/--demo по-прежнему нужен явный совместимый EMU_EXECUTABLE;
при другом runtime задавайте и его matching compiler/header overrides.
