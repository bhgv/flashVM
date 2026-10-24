# Собственный Limbo: приоритет и контракт интерфейсов

[English](OWN_COMPILER.md) · [Coco/R API](frontend/README.ru.md) ·
[Внешние модули Dis/SWF](examples/external-modules/README.ru.md) ·
[Debug info](../../fir/v1/doc/SOURCE_DEBUG.ru.md)

Основная реализация — наш C99/Coco/R frontend Limbo и FIR-конвейер.
C- и Limbo-версии из tools/limbo-bootstrap/upstream служат bootstrap и базой для
переноса, особенно stubs.b и sbl.b. Они не переименованы в «готовый наш компилятор».
Источник: https://github.com/bhgv/Inferno-OS-bhgv, experiment; commit — в manifest.

## Уже работающие шаги

- Широкая Coco/R-грамматика строит owned concrete tree и каталог явных `.m`
  деклараций с позициями. Это отдельный этап от семантического понижения.
- `limbo_bindings` читает этот каталог и генерирует внешние C99/Python-заготовки,
  AS-адаптер реализации внешнего модуля, AS-прокси Dis-модуля и нейтральный JSON.
  Один и тот же `.m` описывает Dis либо не-Dis реализацию и вызовы из AS.
- Числовой Limbo→FIR компилирует примеры с лексическими областями. `-g` выдаёт
  FIRDBG1: точный исходник, SSA origins до оптимизации, переменные и события scope.
  Это пока не карта окончательных Dis/SWF/JIT PC.
- Локальный временный bootstrap собирает сложный Limbo-код IDE в Dis/.sbl, пока
  наше понижение/backend не покрывают его. Готовый внешний binary SDK не нужен.

## Обязательное направление

1. Все новые парсеры — Coco/R, грамматики читаемые и комментированные.
2. Полная семантика, типы, модули и корректные layout в собственном Limbo.
   Используются обе upstream-версии; Limbo особенно важен для переносимого self-host.
3. Сохранение/релокация debug origins через FIR optimizer, объекты, линкер,
   Dis/SWF/JIT; функции, лексические переменные и source maps.
4. Генерация внешних модулей по `.m` не только на C, но и на AS. Из того же
   заголовка AS получает прокси Dis-модуля. Нужны реальные signatures и marshaling
   значений/capabilities, не копирование private frame/heap pointers.
5. Limbo/Dis и AS/SWF взаимодополняют друг друга, сохраняя владение, кучи и
   планирование за проверяемыми типизированными каналами/модулями. Возможное
   последующее объединение отдельно проектируется и проверяется.

AS-адаптеры сейчас — исходники и транспортный контракт, не уже подключённый
native module loader. C-thunks — заготовки, не угаданный Dis ABI. Includes,
aliases, ADT layouts, structural signatures, GC maps и сложные значения требуют
семантического слоя/host bridge. Полное AS-понижение/ABC и собственный полный
Limbo→Dis backend продолжают разрабатываться.
