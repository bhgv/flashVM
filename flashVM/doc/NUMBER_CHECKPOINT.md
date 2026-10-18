# v0.10 validation / Проверки v0.10

Added native boxed Number, primitive unwrapping, constants, valueOf/toString,
integer radix formatting, GC-traced constructor capture, instanceof recognition
and serializer state/method restoration. Number() retains primitive conversion.
See STANDARD_NUMBER.md and STANDARD_NUMBER.ru.md for the supported subset.

Добавлены нативный объект Number, распаковка числа, константы, valueOf/toString,
целочисленное форматирование по основаниям, корни GC, instanceof и сериализация.
Функция Number() сохраняет преобразование в примитив. Ограничения описаны в
STANDARD_NUMBER.md и STANDARD_NUMBER.ru.md.

Linux x86_64, GCC/G++ 13.3.0, C89/C++98, CMake/CTest 3.31.10:

| Profile / Профиль | Passed / Пройдено |
| --- | ---: |
| Headless + SQLite + OS ports | 143/143 |
| AGG + fonts/codecs + SQLite + OS ports | 153/153 |
| Cairo + fonts/codecs + SQLite + OS ports | 153/153 |
| No OS/graphics/fonts/codecs/SQLite | 135/135 |
| Strict runner, ASan heap/global + UBSan, no SQLite | 140/140 |

Tests cover constructor defaults and undefined/NaN, Number() versus new Number(),
constants, prototype value, base 2/10/16/36, largest supported exact integer,
infinity and negative zero, unsupported fractions, invalid radix, numeric
unwrapping, prototype identity, cycles, queue restoration after GC, retained
constructor after removing its global, atomic installation under allocation
failure and file restoration in independent processes without standard-library
installation at the destination. File coverage extends the existing Error
writer/reader test to include a boxed Number packet.

Проверяются значения по умолчанию, undefined/NaN, различие функции и конструктора,
константы, прототип, основания 2/10/16/36, граница точных целых, бесконечность,
отрицательный ноль, неподдерживаемые дроби и неверное основание, распаковка числа,
идентичность прототипа, циклы, очередь после GC, корень конструктора после удаления
глобального имени, атомарная установка при отказах памяти и чтение файла другим
процессом без стандартной библиотеки.

The sanitizer run explicitly disables all ASan stack checks and LeakSanitizer,
retaining heap/global ASan and UBSan. Historical container_exceptions is UBSan
only. SQLite is excluded from that sanitizer profile but tested normally above.
These are the same environment restrictions as v0.9; unrestricted-host sanitizer
coverage and Windows/macOS/Android execution remain outstanding.

В санитайзерной сборке явно выключены все проверки стека ASan и LeakSanitizer;
проверки кучи/глобальной памяти ASan и UBSan включены. Старый container_exceptions
проверяется только UBSan. SQLite проверен в обычных сборках, но отключён в профиле
санитайзеров. Требуются дополнительные проверки без ограничений среды и реальные
запуски Windows/macOS/Android.

Generated logs, copied profiles and JSON reports are delivered separately in
FlashVM-validation-v0.10.zip and are not committed. The source archive includes
the complete internal Git history. Before this work, the explicit mirror push
and subsequent fetch verified v0.9 commit b2f680dd0e7d1ff633ee7031e28fb09f063f193c.
No v0.10 mirror push is automatic. This is not completion of Flash/SWF 8/JIT.

Логи и технические результаты находятся в отдельном архиве и не коммитятся.
Основной архив содержит полную историю внутреннего Git. До начала работ зеркало
обновлено и проверено на v0.9. Автоматической отправки v0.10 нет; полная реализация
Flash/SWF 8/JIT не заявляется.
