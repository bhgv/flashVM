# URL-кодирование счётных строк

Исполняемый пример C89 вызывает `escape`/`unescape` через глобальный C API.
Проверяет полный круг UTF-16: китайские символы, пунктуация, NUL и Z после него.
Сеть, устройство, окно и SWF не нужны. [English и таблица функций](README.md).

Из компонента FlashVM:

```sh
cmake -S . -B build -DFVM_BUILD_EXAMPLES=ON
cmake --build build --target flashvm_examples_embedding_url_encoding_main
./build/flashvm_examples_embedding_url_encoding_main
```

Ожидается:

```text
encoded: %E4%B8%AD%E6%96%87%20%2B%20%40%00Z
roundtrip units: 8 (including NUL)
```

[main.c](main.c) объясняет каждую непустую строку. [`fvm_call`](../../../include/flashvm/value.h#L204) работает синхронно,
может выделять память или вызывать пользовательский ToString, но не открывает
сеть. Строки принадлежат контексту; копия UTF-16 — вызывающему коду. Между
операциями примера нет сборки; при последующем traced-string GC длительно
хранимые результаты должны быть достижимы из VM. На любом пути сначала
уничтожается контекст, затем runtime.

Индивидуальные контракты функций/владение/блокировки:
[таблица API](README.md#apis-ownership-and-blocking).
Ограничения, ошибки, GC, бюджет и неподдержанный повреждённый ввод:
[RU](../../../doc/URL_FUNCTIONS.ru.md) / [EN](../../../doc/URL_FUNCTIONS.md).
Полная эталонная, codepage- и межплатформенная совместимость не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:204](../../../include/flashvm/value.h#L204) |
<!-- source-links:end -->
