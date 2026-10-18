# C++98 channel wrapper / Обёртка канала на C++98

A minimal RAII wrapper releases one owned channel reference when leaving scope.
Copying is deliberately disabled to avoid accidental double-release;
Explicitly call `fvm_channel_value_copy()` to retain another reference to the same
channel; this does not duplicate its queue or messages.

Минимальная RAII-обёртка освобождает одну ссылку на канал при выходе из области видимости.
Копирование запрещено, чтобы избежать двойного освобождения. Для создания
дополнительной владеющей ссылки на тот же канал вызывайте
`fvm_channel_value_copy()`: очередь и сообщения при этом не копируются.

<!-- example-catalog-api:start -->
## Назначение и особенности

Оборачивает владеющий дескриптор канала в C++98 RAII-класс. Запрещает неявное копирование и освобождает ссылку при выходе из области видимости; полезен для безопасного управления каналом в C++-хосте.

**Текущая CMake-цель:** `flashvm_examples_embedding_channel_cpp98_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке описано действие каждой
функции и дан переход к руководству; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Назначение и документация |
| --- | --- |
| [`fvm_channel_value_copy`](../../../include/flashvm/channel_value.h#L26) | Удерживает дополнительную ссылку на тот же канал; очередь и сообщения не копируются. Каждую успешную копию нужно освободить. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_copy](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_copy). |
| [`fvm_channel_value_create`](../../../include/flashvm/channel_value.h#L22) | Создаёт новый внутрипроцессный канал с одной владеющей ссылкой; сохраняет дескриптор в out. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_create](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_create). |
| [`fvm_channel_value_receive`](../../../include/flashvm/channel_value.h#L34) | Получает примитивное сообщение; ждёт при пустой очереди. Полученная строка заимствована у канала. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_receive](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_receive). |
| [`fvm_channel_value_release`](../../../include/flashvm/channel_value.h#L29) | Освобождает одну ссылку и обнуляет дескриптор; последний release уничтожает канал, а не просто закрывает его. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_release](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_release). |
| [`fvm_channel_value_send`](../../../include/flashvm/channel_value.h#L31) | Копирует примитивное сообщение в общий канал; ждёт при заполненной ограниченной очереди. Исходное значение остаётся у вызывающего. Подробнее: [CHANNEL_VALUES.md#fvm_channel_value_send](../../../doc/CHANNEL_VALUES.md#fvm_channel_value_send). |
| [`fvm_value_boolean`](../../../include/flashvm/value.h#L44) | Записывает тег Boolean и нормализует значение: 0 становится false, любое ненулевое число — true. Подробнее: [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views). |

Локальная RAII-обёртка вызывает C API владения; её запрет копирования не является новым ABI VM. См. [CHANNEL_VALUES.md](../../../doc/CHANNEL_VALUES.md#api).

[Описание в общем каталоге](../../README.md#example-embedding-channel_cpp98).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:26](../../../include/flashvm/channel_value.h#L26) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:22](../../../include/flashvm/channel_value.h#L22) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:34](../../../include/flashvm/channel_value.h#L34) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:29](../../../include/flashvm/channel_value.h#L29) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:31](../../../include/flashvm/channel_value.h#L31) |
| `fvm_value_boolean` | function | [include/flashvm/value.h:44](../../../include/flashvm/value.h#L44) |
<!-- source-links:end -->
