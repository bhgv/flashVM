# Первые исполняемые SWF / First executable SWF examples

Эти два небольших примера проходят настоящий путь **AS2 → FIR → SWO → SWF** через
собственные инструменты репозитория. Это числовое подмножество языка: пример не
доказывает поддержку произвольного AS2, AS3/ABC, окон или полного Flash Player.

| Файлы | Что изучаем | Ожидаемый trace |
| --- | --- | --- |
| `constant.as`, `constant.fir`, `constant.swf` | Числовой литерал и вызов trace | `123` |
| `arithmetic.as`, `arithmetic.fir`, `arithmetic.swf` | Скобки, вычитание, умножение и FIR-операции | `42` |

В исходниках прокомментирована каждая строка кода. `trace(value)` получает одно
вычисленное числовое значение и передаёт его callback-у вывода хоста. Примеры не
создают внешних ресурсов, каналов или нативных указателей. В FIR соответствующие
инструкции читаются в порядке вычисления; SSA-имена связывают значения с trace.
Файлы SWF — воспроизводимые учебные продукты, а не журналы успешности тестов.

После обычной сборки локальный пакет уже содержит SWF и исходники рядом с runner:

```sh
cd out/flashvm/no-graphics-headless
./flashvm constant.swf
./flashvm arithmetic.swf
```

Каталог зависит от выбранного профиля: также возможны `agg-headless` и
`cairo-headless`. `flashvm` здесь — настоящий консольный reference SWF runner;
он не открывает окно. На Windows имя программы имеет суффикс `.exe`.

Повторная генерация из корня репозитория после сборки FIR и SWL:

```sh
FIR_SOURCE="$PWD/out/fir/fir1_source" FIR_TOOL="$PWD/out/fir/fir1" \
SWL_TOOL="$PWD/out/linker/swl" bash examples/firststeps/build.sh
cmp examples/firststeps/constant.swf build/firststeps/constant.swf
cmp examples/firststeps/arithmetic.swf build/firststeps/arithmetic.swf
```

`build.sh` сохраняет промежуточные файлы в `build/firststeps` и задаёт writable
TMPDIR для временного FIR. Поставляемые файлы в исходном каталоге не перезаписываются.
Для ручного изучения используйте `fir1 check`, `fir1 opt`, `fir1 object`, затем
`swl link`; это отдельные инструменты с явными промежуточными форматами.

English: these reproducible numeric AS2 samples use the real Coco/R/FIR/SWL
pipeline. Run the two packaged SWFs to obtain `123` and `42`. The console runner
is not a GUI player. Rebuild with the explicit tool paths above; generated files
stay in the build directory. This is numerical-subset acceptance, not full
source-language or Flash/platform compatibility.
