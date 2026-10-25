# Native tool tutorials / Учебные примеры инструментов

These examples belong beside the installed native tools, not beside an external emu.
Все результаты создавайте в отдельном рабочем каталоге, не внутри исходников.

## mk

```sh
MK="$PWD/out/mk/mk"
SAMPLES="$PWD/out/mk/examples"
mkdir -p build/mk-tutorial
cd build/mk-tutorial
"$MK" -f "$SAMPLES/hello.mkfile"
printf 'input for the pattern rule\n' > input.txt
"$MK" -f "$SAMPLES/copy.mkfile"
```

`hello.mkfile` создаёт hello.txt. `copy.mkfile` показывает настоящий mk pattern rule
и `$prereq`/`$target`, а не диалект GNU Make. Обе recipe выполняются реальным mk.

## iyacc

```sh
PACKAGE="$PWD/out/iyacc"
mkdir -p build/iyacc-tutorial
cd build/iyacc-tutorial
ROOT="$PACKAGE" "$PACKAGE/iyacc" -d "$PACKAGE/examples/sum.y"
cc y.tab.c -o sum
printf '19+23\n' | ./sum
```

Expected output: `42`. Для number.y подайте `123` и получите `123`.
ROOT обязателен: установленный skeleton лежит в utils/lib/yaccpar относительно
этого каталога. Это демонстрация временного legacy bootstrap-инструмента, не
замена Coco/R для наших компиляторов. Каждая функция и строка C-действия объяснены.

## Coco/R

```sh
PACKAGE="$PWD/out/cocor"
mkdir -p build/cocor-tutorial
cd build/cocor-tutorial
cp "$PACKAGE/examples/number.atg" .
CRFRAMES="$PACKAGE/frames" "$PACKAGE/cocor" number.atg
```

Генерируются настоящие parser/scanner C-файлы. name.atg — второй пример.
Тот же небольшой ATG-профиль проверяется установленным cocor-limbo bootstrap;
наличие сохранённого cocor.dis не означает готовый embedded Dis runner.
Для полного языка используйте собственные compiler/*/frontend/*.atg и их README.

## Limbo bootstrap

The package contains arithmetic.b/arithmetic.m and the module headers.
Пример компиляции из out/limbo-bootstrap:

```sh
./limbo-bootstrap -I module -I examples -o arithmetic.dis examples/arithmetic.b
```

Это настоящий продукт компилятора, но для исполнения требуется совместимый Dis
хост; один bootstrap не предоставляет runtime. Основной компилятор проекта —
собственный Coco/R Limbo/FIR, остающийся в разработке.
