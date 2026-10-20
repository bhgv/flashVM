# Инструменты FIR v1

[English](README.md). Реализация **0.3.2**, текстовый формат **FIR 1.0**.
Код — C99. Оба парсера генерируются внешним
[bhgv/coco-r-C](https://github.com/bhgv/coco-r-C), тем же генератором, что
используют синтаксические фронтенды AS1/AS2/AS3/Limbo. Сгенерированный код остаётся
в каталоге сборки и не хранится в Git.

## Сборка из корня репозитория

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
COCO_DIR="$TMPDIR/coco-r-C"
git clone https://github.com/bhgv/coco-r-C.git "$COCO_DIR"
make -C "$COCO_DIR/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$COCO_DIR" \
  -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/fir1
ctest --test-dir build/fir1 --output-on-failure
```

Linux/Termux: нужны компилятор C99, make, CMake; Python 3 нужен для тестов.
LLVM, SDL и заголовки VM для сборки инструментов не нужны. В Windows укажите
реальный путь к внешнему генератору и используйте C99 toolchain. Windows и
физический Android в этом выпуске не проверены. Отдельно скопированный или
переименованный v1 собирается с теми же параметрами; сохраните корневую MIT LICENSE.

## Руководства

- [Формат и примеры](doc/FORMAT.ru.md): реализованная грамматика и семантика.
- [Покрытие](doc/COVERAGE.ru.md): исходные профили, AVM1 и будущие возможности.
- [Компилятор и API](compiler/README.ru.md): параметры CLI и интерфейсы C99.
- [Peephole](optimiser/README.ru.md): преобразования и ограничения.
- [Примеры](examples/README.ru.md): четыре адаптера, ветвления и каналы.
- [Проверки и хост выполнения](doc/TESTING.ru.md).

## Просмотр

```sh
python -m pip install -r flashVM/doku_viewer/requirements.txt
python fir/v1/view_docs.py --no-browser --port 8766
# Или: python view_docs.py
```

Оба просмотрщика выводят все руководства и примеры FIR. Локальные launchers
есть также в `fir/`, `doc/`, `compiler/`, `optimiser/`, `examples/`. Они используют
общий движок и границу репозитория, поэтому ссылки на API FlashVM работают.
Для отдельной сборки инструментов движок просмотра не требуется; документы
в отделённой копии можно читать как Markdown.

## Изменение backend в 0.3.2

`fir1 swf` и `fir1 actions` удалены. Используйте `fir1 object INPUT OUTPUT.swo`,
затем [SWL](../../linker/README.ru.md): `swl link OUTPUT.swf INPUT.swo`.
Прежние команды в исторических примерах заменяются этим двухэтапным путём.
Для тестов выполнения задайте `FIR_SWL` вместе с `FIR_FLASHVM_RUNNER`.

[C API и встраивание](compiler/API.ru.md).

[Сборка и объектный pipeline](doc/PIPELINE.ru.md). [Пример IDE](../../linker/examples/ide/README.ru.md).
