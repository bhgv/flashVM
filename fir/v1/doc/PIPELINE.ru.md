# Исходник → FIR → оптимайзер → SWO → линкер → SWF

Команды из корня репозитория. В Termux: `pkg install git clang make cmake python`.
Генератор — внешний bhgv/coco-r-C; его исходники/runtime не копируются в flashVM.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
git clone https://github.com/bhgv/coco-r-C.git "$TMPDIR/coco-r-C"
make -C "$TMPDIR/coco-r-C/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$TMPDIR/coco-r-C" \
  -DCOCOR_EXECUTABLE="$TMPDIR/coco-r-C/sources/cocor"
cmake --build build/fir1
cmake -S linker -B build/linker
cmake --build build/linker
build/fir1/fir1_source as2 --input fir/v1/examples/as2/variables.as build/variables.fir --module variables
build/fir1/fir1 check build/variables.fir
build/fir1/fir1_opt build/variables.fir build/variables.opt.fir
build/fir1/fir1 object build/variables.opt.fir build/variables.swo
build/linker/swl archive build/library.swa build/variables.swo
build/linker/swl link build/variables.swf build/library.swa
FIR_SWL="$PWD/build/linker/swl" ctest --test-dir build/fir1 --output-on-failure
ctest --test-dir build/linker --output-on-failure
```

Ожидаемый trace: 36, 42. SWO1 — типизированный по целевому профилю объект
AVM1; он не содержит SWF-контейнера. SWA1 включает объекты целиком. Несколько
модулей исполняются по порядку входов; дубликаты имён запрещены. Межмодульные
вызовы функций, imports/exports функций и их релокации пока не реализованы.
Для каналов FIR требуется `swl link ... --extensions ...` и установка расширений
в host. [Ресурсы SVF/JPEG/WAV](../../../linker/ASSETS.ru.md) добавляются линкером,
не компилятором. Оптимизация явная, формат текста остаётся `FIR 1.0`.

[Пример C IDE](../../../linker/examples/ide/README.ru.md) связывает все библиотеки
в один процесс. [API](../compiler/API.ru.md). Верификатор дополнительно проверяет
индексы, размеры таблиц, арность и имена IR, созданного из C. Разбор исходников
пока требует внешней сериализации, поэтому API не следует вызывать параллельно
без мутекса. Полные языки, ABC, invoke/catch и параметры блоков остаются будущей
работой. Windows/физический Android в этой серии не проверялись.
