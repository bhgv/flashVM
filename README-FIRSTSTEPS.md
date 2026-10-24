# Первые шаги: сборка и запуск

[English](README-FIRSTSTEPS.en.md) · [Обзор](README.ru.md) ·
[Подробная сборка и кросс-компиляция](BUILDING.ru.md)

Запускайте команды из корня репозитория. Общая сборка необязательна:
каждый проект сохраняет собственный CMakeLists.txt и независимые зависимости.
Скрипт рассчитан на нативную сборку Bash + CMake 3.16+, C99/C++ компилятором,
Make или Ninja и Python 3 для тестов. На Termux подготовьте инструменты:

```bash
pkg install git clang make cmake python
```

Физический Android этой серией не проверялся. Все внешние исполняемые файлы
должны соответствовать вашей ОС и архитектуре; Linux amd64 SDK нельзя
использовать как Android arm64 SDK.

## Меню выбора компонентов

```bash
python3 build-menu.py
```

Введите один номер (например, `4` — линкер), несколько (`1,4,6`) или имена
(`vm linker ide`). `all` выбирает всё, `q` завершает меню. Затем можно включить
тесты и консольный пример IDE. Скрипт сразу запускает выбранную сборку,
передаёт её вывод в терминал и возвращает её код завершения.
Каждый пункт — отдельный компонент; `compilers` собирает четыре фронтенда вместе.

Без интерактивных вопросов:

```bash
python3 build-menu.py --projects linker --tests
python3 build-menu.py --projects vm,linker --jobs 4
python3 build-menu.py --projects ide --demo
python3 build-menu.py --projects all --build-root build/release
# Только показать команду, ничего не собирать:
python3 build-menu.py --projects 1,4 --dry-run
```

Меню использует тот же `build-workspace.sh` и те же переменные SDK, описанные
ниже. Python — инструмент сборки на хосте; для исполнения Limbo-ядра он не нужен.

## Подготовить внешние компиляторы

Парсеры используют внешний Coco/R. Подготовьте его один раз:

```bash
git clone https://github.com/bhgv/coco-r-C.git ../coco-r-C
make -C ../coco-r-C/sources -f unix.mk
export COCOR_ROOT="$(cd ../coco-r-C && pwd)"
export COCOR_EXECUTABLE="$COCOR_ROOT/sources/cocor"
```

IDE сейчас собирается внешним нативным компилятором Limbo. Укажите пути
к своему готовому Inferno SDK; команды ниже содержат заменяемый путь:

```bash
export INFERNO_SDK=/absolute/path/to/inferno
export LIMBO_EXECUTABLE="$INFERNO_SDK/Linux/amd64/bin/limbo"
export EMU_EXECUTABLE="$INFERNO_SDK/emu/Linux/o.emu"
export LIMBO_MODULE_DIR="$INFERNO_SDK/module"
```

Пути Linux/amd64 приведены как пример, не как универсальная схема SDK.
[Инструкции reference runtime](ide/dis/REFERENCE_RUNTIME.md) и
[сборка Limbo-модуля](ide/dis/README.md) описывают отдельные требования.
Целевой перенос Dis ведётся по
[Inferno experiment](https://github.com/bhgv/Inferno-OS-bhgv/tree/experiment).
Корневой скрипт не скачивает и не собирает внешнюю ОС и не встраивает её в FlashVM.

## Собрать и регулярно пересобирать

```bash
./build-workspace.sh --help
./build-workspace.sh --jobs 2
# Повторите ту же команду после обновления исходников: сборка инкрементальная.
./build-workspace.sh --jobs 2 --tests
```

По умолчанию собираются VM, четыре синтаксических фронтенда, FIR-инструменты,
SWF-линкер, читатель отладочных символов sblmap и Limbo-ядро IDE. SDK проверяются до начала сборки. Первая ошибка
останавливает работу; частичные результаты сохраняются для диагностики.
Скрипт ничего не устанавливает, не удаляет, не коммитит и не запускает по расписанию.

```bash
# Только VM и линкер: Coco/R и Limbo не нужны.
./build-workspace.sh --projects vm,linker
# Только компиляторы и оптимизатор.
./build-workspace.sh --projects compilers,fir
# Отдельный каталог и конфигурация.
./build-workspace.sh --projects linker --build-root build/debug --config Debug --tests
```

Временные файлы находятся в build-root/tmp. Параметры SDK можно хранить
в своём shell-скрипте окружения. После смены SDK/компилятора используйте новый
`--build-root`, чтобы не смешивать несовместимые CMake-кэши.

| Результат (обычный Unix Makefiles/Ninja) | Назначение |
| --- | --- |
| build/workspace/vm/flashvm_reference_swf | Консольное исполнение SWF и trace |
| build/workspace/compilers/as1/frontend/as1_parse | Распознавание синтаксиса AS1; аналоги в as2, as3, limbo |
| build/workspace/fir/fir1_source | Ограниченное по покрытию понижение AS/Limbo в FIR |
| build/workspace/fir/fir1 | Проверка FIR и генерация SWO |
| build/workspace/fir/fir1_opt | Явная оптимизация FIR |
| build/workspace/linker/swl | SWO/SWA → SWF8 |
| build/workspace/symbols/sblmap | Проверка/чтение Limbo SBL |
| build/workspace/ide/idecore.dis, idecore.sbl | Ядро IDE и отладочные данные |

Многоконфигурационные генераторы могут добавлять подкаталог Release/Debug.
Синтаксический разбор не означает полную реализацию языка: см.
[покрытие компиляторов](compiler/PARSER_COVERAGE.md).

## Запустить доступную часть IDE

```bash
./build-workspace.sh --projects ide --demo
```

Это собирает Limbo-ядро и запускает комментированный
[пример навигации](ide/dis/examples/navigation/README.ru.md) в двух режимах
внешнего reference emu. Ожидается `PASS idecore navigation 16 checks`.
Пример использует типизированные каналы: открывает документ, ищет текст,
редактирует его и отклоняет устаревшую ревизию. `idecore.dis` — библиотечный
модуль, его нельзя запускать как самостоятельное приложение с init.

Полноценное графическое IDE и встроенный Dis/JIT ещё разрабатываются.
Данный запуск проверяет доступное ядро и клиентский пример, а не готовый
графический редактор и не доказательство нового нативного JIT.
Для разработки смотрите [протокол](ide/dis/PROTOCOL.md),
[архитектуру](ide/ARCHITECTURE.ru.md) и [отладку](ide/DIS_ACCEPTANCE.md).

## Прогнать исходник через инструменты

После сборки fir,linker,vm:

```bash
build/workspace/fir/fir1_source as2 --input fir/v1/examples/as2/variables.as build/workspace/variables.fir --module variables
build/workspace/fir/fir1 check build/workspace/variables.fir
build/workspace/fir/fir1_opt build/workspace/variables.fir build/workspace/variables.opt.fir
build/workspace/fir/fir1 object build/workspace/variables.opt.fir build/workspace/variables.swo
build/workspace/linker/swl archive build/workspace/library.swa build/workspace/variables.swo
build/workspace/linker/swl link build/workspace/variables.swf build/workspace/library.swa
build/workspace/vm/flashvm_reference_swf build/workspace/variables.swf
```

Ожидаемые trace: 36, 42. [Подробности конвейера](fir/v1/doc/PIPELINE.md),
[примеры линкера](linker/examples/README.md),
[host-инструменты IDE](ide/README.ru.md).
Для окна проигрывателя отдельно соберите SDL2 frontend по
[его руководству](flashVM/frontend/README.md); он требует SDL2 SDK.

```bash
# Один фронтенд или несколько отдельных языков:
python3 build-menu.py --projects as1
python3 build-menu.py --projects as2,limbo
```

`all` включает группу `compilers`; отдельные языки повторно не собираются.

## История правок и точные ошибки компилятора

```bash
python3 build-menu.py --projects ide --tests --demo history
build/workspace/fir/fir1_diagnostic_example
python3 ide/build.py ide/examples/numeric/program.as build/workspace/demo.swf --language as2 --compiler build/workspace/fir/fir1_source --ir build/workspace/fir/fir1 --linker build/workspace/linker/swl --structured-diagnostics
```

Первый запуск требует Limbo/emu/module SDK и показывает Undo/Redo через каналы.
Второй — комментированный C99-клиент ожидаемой ошибки (нужна сборка fir).
Третий публикует SWF только после успешной цепочки компиляции и возвращает JSON;
ошибки исходника получают проверенные байтовые/символьные диапазоны и SHA-256
снимка. Это host-инструмент; Python не входит в runtime IDE.
[История](ide/dis/HISTORY.ru.md) ·
[Схема ошибок](fir/v1/doc/SOURCE_DIAGNOSTICS.ru.md).
