# Структурные кодеки и инспектор Dis64

[English](README.md) · [C99-пример с комментариями](examples/main.c) ·
[Контракт контейнера](../upstream/doc/dis64-envelope.md) ·
[Строковый пул](../upstream/doc/dis64-strings.md) ·
[Принятый registry](../upstream/doc/dis64-registry.json)

`dis64inspect` читает согласованные заголовок, manifest обязательных расширений
и каталог секций. JSON явно сообщает `envelope_only:true` и
`execution_supported:false`. Успешный отчёт не доказывает исполнимость module,
types, code или imports. Старые NC64/Dis не конвертируются. Обязательное
расширение, не реализованное инструментом, отвергается.

C99 API — те же выбранные заголовки, которыми пользуется инспектор:
[dis64.h](../upstream/include/dis64.h),
[dis-string-pool.h](../upstream/include/dis-string-pool.h),
[dis-rune.h](../upstream/include/dis-rune.h). Это header-only кодек без
выделения памяти. Вызывающий код передаёт стабильные доступные входы, отдельный
выход и явные лимиты. Заимствованные views живут не дольше неизменяемого входа.
Здесь нет доступа к куче VM, передачи native pointers и привязки к кадрам.

[Построчно прокомментированный пример](examples/main.c) создаёт структурный
контейнер через эти API. M/C/T — метки fixture, а не программа или настоящие
module/type descriptors. Строковый пул настоящий: два разных ID содержат
одинаковый Rune со всеми сохранёнными битами. Проверяются запись, чтение,
lookup и знаковое сравнение без normalization или interning.

## Сборка исходников

```sh
cmake -S disVM/envelope -B build/envelope
cmake --build build/envelope
ctest --test-dir build/envelope --output-on-failure
build/envelope/dis64inspect build/envelope/sample.dis64
build/envelope/dis64inspect build/envelope/sample.dis64 --rewrite build/envelope/copy.dis64
```

Самостоятельной сборке нужны C99 и CMake; Python используется только host-тестом.
Core/runtime собирают тот же инструмент и пример. Не требуются bootstrap
Limbo, генератор Coco/R, FlashVM, SDL или целая Inferno OS. Для одного
инспектора в исходниках upstream также есть небольшой Makefile.

## Установленный SDK

Из каталога пакета `disvm-editor`, `disvm-runtime`, `disvm-host` или `disvm`:

```sh
./run-dis64-envelope.sh
./dis64inspect examples/dis64-envelope/sample.dis64
mkdir -p work
cc -std=c99 -Iinclude/dis64 examples/dis64-envelope/main.c -o work/envelope-example
./work/envelope-example work/sample.dis64
./dis64inspect work/sample.dis64 --rewrite work/copy.dis64
```

Launcher проверяет установленный fixture без checkout исходников. Build-menu
показывает отдельный пункт **Dis64: инспектор контейнера (демо)**. Это не новая
исполняемая программа DisVM и не отладчик IDE. C-пример проверяет кодеки до
открытия явно указанного файла; итоговый I/O проверяется, но не является
атомарной файловой транзакцией.

## Границы реализации и проверок

Envelope проверяет version/ABI/flags, имена и policy расширений, плотные extents,
singletons, обязательные kinds и лимиты. Pool проверяет count, все non-nil Rune
records и точный расход bytes; сохраняет ID, повторные строки и signed32 order.
Отказы не публикуют частичный результат. Поиск ID пока линейный; будущий loader
может отдельно построить проверенный индекс offsets.

Тесты запускают настоящий собранный инспектор и C99-клиент, сравнивают bytes/JSON,
проверяют отказ до перезаписи файла и явное отсутствие executable support.
Upstream fixtures проверяют сами кодеки и композицию pool→typed case. SDK
переносится в другой каталог; пример перелинковывается только с его заголовками.

Module/type/code/import payloads и первый исполняемый pilot находятся в
[прямом обмене](../upstream/doc/dis64-flashvm-payload-response.md). Эти
структурные инструменты их не реализуют. Новый VM ABI, ref-result GC/unwind
и физический запуск ARM64 этой серией не объявляются готовыми.

Native-сборка создаёт пример C99-клиентом. Cross-сборка не запускает целевые
бинарники: installer декодирует проверенный sample.dis64.hex уже используемым
host Python. Установленному demo нужны только native-инспектор и fixture,
без Python. Запуск на другой платформе проверяется отдельно.
