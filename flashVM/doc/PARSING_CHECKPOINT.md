# v0.11 validation / Проверки v0.11

Added global parseInt/parseFloat prefix parsing and String conversion, atomically
installed with the native standard library. Native-call name lookup and callback
allocation failure now stay within the C ABI. No new wire kinds or ABI layouts
were introduced. See GLOBAL_PARSING.md and GLOBAL_PARSING.ru.md for limitations.

Добавлены глобальные parseInt, parseFloat и String с атомарной установкой.
Отказы памяти при поиске имени и выполнении нативного вызова не выходят за C ABI.
Формат передачи и структуры ABI не изменены. Ограничения описаны в документации.

Linux x86_64, GCC/G++ 13.3.0, strict C89/C++98, CMake/CTest 3.31.10:

| Profile / Профиль | Passed / Пройдено |
| --- | ---: |
| Headless + SQLite + OS ports | 145/145 |
| AGG + fonts/codecs + SQLite + OS ports | 155/155 |
| Cairo + fonts/codecs + SQLite + OS ports | 155/155 |
| No OS/graphics/fonts/codecs/SQLite | 137/137 |
| Strict runner, ASan heap/global + UBSan, no SQLite | 142/142 |

New coverage includes executable AVM1 CallFunction records, an independent
integer-encoding oracle for every radix 2–36 and samples 0–200, legacy octal and
hexadecimal inference, radix ToInt32 wrapping/truncation, incomplete exponents,
trailing text, Unicode whitespace, missing arguments, native conversion throws,
installation conflicts and injected allocation failures for all three functions.

Новые проверки включают исполняемые AVM1 CallFunction, независимый кодировщик целых
для оснований 2–36 и чисел 0–200, автоопределение восьмеричной/шестнадцатеричной
формы, ToInt32 основания, незавершённые экспоненты, хвосты, Unicode-пробелы,
пропущенные аргументы, исключения преобразования, конфликты установки и отказы
памяти в каждой из трёх функций.

The Cairo suite was repeated after the environment removed executable permission
from the raster test; the final suite passed. The sanitizer profile explicitly
uses --no-asan-stack: all ASan stack checks and LeakSanitizer are disabled; heap/
global ASan and UBSan remain enabled. Historical container_exceptions is UBSan
only. SQLite is omitted from that profile and tested in the ordinary builds.
Full stack/LSan checking on an unrestricted host and real Windows/macOS/Android
execution remain outstanding. Numeric rounding and SWF-version-dependent parsing
are not certified by these tests.

Cairo проверен повторно после восстановления прав запуска теста растеризации.
Итоговый набор прошёл. В санитайзерном профиле выключены все проверки стека ASan
и LeakSanitizer, сохранены проверки кучи/глобальной памяти и UBSan. Старый
container_exceptions использует только UBSan; SQLite отключён в этом профиле и
проверен обычными сборками. Требуются проверки без ограничений среды и на целевых
платформах. Полное округление и различия версий SWF этими тестами не доказаны.

Generated logs/profiles/JSON are delivered in FlashVM-validation-v0.11.zip,
separately from the source archive with full internal Git history. Before this
work, the authorized mirror update was fetched and verified at v0.10 commit
e6ddf93b73e936160d918fb0b70f26e7acf9093a. v0.11 is not pushed automatically.

Логи, профили и JSON не коммитятся, а выдаются отдельным архивом. Основной архив
содержит исходники и историю внутреннего Git. Зеркало по прямой команде проверено
на v0.10 до начала работ. v0.11 автоматически не отправляется. Полная реализация
всех объектов Flash, SWF 8 и JIT ещё не завершена.
