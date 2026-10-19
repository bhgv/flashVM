# Исходник → FIR → SWF

Команды выполняются из корня репозитория. На Termux нужны `git clang make cmake python`.
Генератор внешний, его исходники и runtime не копируются в flashVM.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
git clone https://github.com/bhgv/coco-r-C.git "$TMPDIR/coco-r-C"
make -C "$TMPDIR/coco-r-C/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$TMPDIR/coco-r-C" \
  -DCOCOR_EXECUTABLE="$TMPDIR/coco-r-C/sources/cocor"
cmake --build build/fir1
ctest --test-dir build/fir1 --output-on-failure
build/fir1/fir1_source as2 --input fir/v1/examples/as2/variables.as build/variables.fir
build/fir1/fir1 check build/variables.fir
build/fir1/fir1 opt build/variables.fir build/variables.opt.fir
build/fir1/fir1 swf build/variables.opt.fir build/variables.swf
build/fir1/fir1 actions build/variables.opt.fir build/variables.actions
build/fir1/fir1 --help
build/fir1/fir1_source --help
```

Ожидаемый trace: 36 и 42. Это один кадр FWS8 с AVM1, без графики.
Для AS1/AS3 замените профиль и каталог примера; для Limbo используйте `limbo`
и `fir/v1/examples/limbo/variables.b` (вывод 36). AS3 здесь — числовой
профиль совместимости AVM1, без ABC/AVM2. `actions` — сырые записи AVM1,
а не объектный формат линкера. Оптимизация отдельная, `swf` её не запускает.

Синтаксические фронтенды `compiler/{as1,as2,as3,limbo}` принимают более широкий
синтаксис и собираются отдельно. Их `--help` не обещает FIR/SWF-генерацию.
Парсер FIR находится в `compiler/fir1.atg`, учебных исходников — в
`compiler/src1.atg`; семантика — C99 `compiler/lower.c`. Добавлять конструкции
нужно одновременно в грамматику, семантику, отрицательные тесты и документацию.

Проверка CTest без `FIR_FLASHVM_RUNNER` не исполняет SWF; подключение VM
описано в [TESTING.ru.md](TESTING.ru.md). Формат остаётся `FIR 1.0`.
Параметры блоков, функции с аргументами, invoke/catch, полные AS/Limbo и ABC
пока не реализованы; это не завершение всего компиляторного роудмапа.
