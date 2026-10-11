# Text API (English)

Public headers: `flashvm/utf8.h`, `flashvm/regex.h`. These functions are
independent of VM instances and can be called from a C89 host or extension.
All text sizes and offsets are **bytes**, unless explicitly called a scalar
(character) index. UTF-8 decoding rejects overlong sequences, surrogate
codepoints, and values above U+10FFFF. An embedded NUL is a valid scalar.

* `fvm_utf8_validate(data, bytes, &scalars)` validates a whole buffer.
* `fvm_utf8_read(data, bytes, &offset, &codepoint)` decodes one scalar and
  advances the offset only after success.
* `fvm_utf8_write(codepoint, output, capacity, &bytes_written)` encodes one
  scalar; caller supplies at least four bytes for all possible scalars.
* `fvm_utf8_offset(data, bytes, scalar_index, &byte_offset)` converts an index
  to a byte offset. Input before the requested index must be valid UTF-8.
* `fvm_regex_search(pattern, data, bytes, &start, &match_bytes)` searches for
  the first bytewise match. Returns 1 (match), 0 (none), -1 (bad arguments),
  or -2 (execution budget exhausted). Pattern must be NUL-terminated.

The bundled `extern/mini_regex` engine supports only `^`, `$`, `.`, `*`,
`+`, `?`, and backslash-escaped literal bytes. It does **not** implement
character classes, groups, alternation, captures, Unicode properties, or
PCRE/POSIX regex syntax. This limited engine is not suitable for processing
untrusted complex patterns as a complete regex solution. No locale-sensitive
case folding, normalization, or grapheme segmentation is implemented.

See `examples/embedding/text/` for C89 usage and its ActionScript comparison.

# Текстовый API (Русский)

Заголовки: `flashvm/utf8.h` и `flashvm/regex.h`. Функции не требуют
создания виртуальной машины и доступны программам и расширениям на C89.
Длины строк и смещения задаются **в байтах**, если явно не указан индекс
Unicode-символа (scalar value). Проверка UTF-8 отклоняет избыточные
кодировки, суррогаты и значения выше U+10FFFF. Нулевой байт внутри
буфера допустим.

* `fvm_utf8_validate` — проверить буфер и посчитать Unicode-символы.
* `fvm_utf8_read` — прочитать один символ и продвинуть позицию при успехе.
* `fvm_utf8_write` — записать символ UTF-8 в предоставленный буфер.
* `fvm_utf8_offset` — получить байтовое смещение по индексу символа.
* `fvm_regex_search` — найти первое совпадение, вернуть байтовые границы.
  Результат: 1 — найдено, 0 — не найдено, -1 — неверные аргументы,
  -2 — исчерпан лимит вычислений.

Встроенный модуль `extern/mini_regex` поддерживает только `^`, `$`, `.`,
`*`, `+`, `?` и экранирование буквальных байтов. Нет групп, альтернатив,
классов символов, захватов, Unicode-свойств и полноценной совместимости
с PCRE/POSIX. Сопоставление происходит по байтам UTF-8. Нормализация,
сравнение без учёта регистра и графемные кластеры пока отсутствуют.

Рабочий пример C89 и сравнение с ActionScript находятся в
`examples/embedding/text/`.
