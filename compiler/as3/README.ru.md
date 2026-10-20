# Синтаксический фронтенд AS3 0.2.0

Грамматика `frontend/as3.atg` — источник парсера Coco/R; драйвер на C99.
Это подмножество синтаксиса AS3, без AST, проверки типов и генерации ABC.
Поддержаны package, import, class/interface, extends/implements, модификаторы,
var/const, сигнатуры, блоки, циклы, исключения и выражения с is/as.
Не поддержаны E4X/XML, metadata, namespace, Vector.<T>, регулярные выражения.
Допустимость сочетаний модификаторов, return и const проверяется будущей семантикой.
Учебный путь AS3 → FIR → AVM1 находится отдельно в `../../fir/v1`; это не AVM2.

```sh
cmake -S compiler/as3 -B build/as3 -DCOCOR_ROOT="$COCO_DIR" -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/as3
ctest --test-dir build/as3 --output-on-failure
build/as3/frontend/as3_parse example.as
```

Внешняя зависимость: [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C).

Примеры: `examples/Counter.as`, `examples/ICounter.as`; каждый проверяется
`as3_parse`. Функциям и параметрам соответствуют FunctionTail/Parameter,
приоритетам — цепочка Assignment…Primary, типам — TypeDeclaration/Member/Signature.
Диагностика содержит файл, строку/колонку в байтах и номер ошибки Coco/R.
Коды завершения: 0 — синтаксис принят, 1 — отвергнут, 2 — ошибка команды/ввода.
UTF-8 и NUL проверяются до сканера. Переименованная копия каталога `as3`
собирается независимо от соседних компонентов.

CLI: `--help`, `--version` (без входного файла; код 0).

## C API

`frontend/frontend.h`: `as3_check(bytes, size, diagnostic)` и
`as3_read(FILE*, diagnostic)` возвращают 1 при принятии, 0 при отказе.
Буфер и поток принадлежат вызывающему коду; библиотека не печатает сообщения,
не закрывает вход и не завершает процесс. Диагностика: код, строка/колонка,
байтовое смещение, сообщение. NULL вместо диагностики допустим. Максимум 2 MiB.
CMake target: `as3_frontend`. Глобальные символы генератора имеют префикс языка;
несколько библиотек могут линковаться в одну IDE. Состояние сбрасывается перед
каждым вызовом, но один парсер требует внешней сериализации (не reentrant).
`tests/embed.c` проверяет восстановление после ошибки в одном процессе.
