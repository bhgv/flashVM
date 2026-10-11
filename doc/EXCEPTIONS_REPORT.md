# v0.8 validation report / Отчёт о проверке

Date / Дата: 2026-10-11. Linux x86_64, GCC/G++ 13.3.0, CMake 3.31.10.
C89/C++98, pedantic-errors; VM containers remain free of STL.

Implemented / Реализовано: standard AVM1 try/catch/finally and throw;
resource exception objects with stable codes, English messages and transport /
source context; pending exception and finally-completion GC roots; copied rich
debugger events. Existing public structures and ABI version are preserved.
See [English](EXCEPTIONS.md), [Русский](EXCEPTIONS.ru.md).

| Profile / Профиль | Result / Результат |
| --- | --- |
| AGG, fonts, image codecs, SQLite, OS ports, render jobs | 146/146 CTest |
| Cairo, fonts, image codecs, SQLite, OS ports, render jobs | 146/146 CTest |
| No graphics/fonts/image codecs, SQLite and OS ports | 136/136 CTest |
| No graphics/fonts/image codecs/SQLite/OS ports/render jobs | 128/128 CTest |
| Strict runner, no graphics/fonts/image codecs/SQLite, OS ports | 133 checks with ASan/UBSan |

After the last code/test changes, the strict sanitizer build's capability
object and broker/network/AVM1 exception tests were rebuilt and passed again.
The full AGG and Cairo suites also passed after the last code changes.
The no-OS profile establishes compilation with those APIs disabled, not actual
Windows/macOS/Android runtime validation. Those target systems were not run.

Sanitizer scope: LeakSanitizer disabled because /proc discovery is unavailable;
container_exceptions uses UBSan only; serialization_c.cpp disables stack ASan
instrumentation explicitly while retaining heap/global ASan and UBSan. No new
sanitizer exclusions were introduced. SQLite's instrumented build did not
complete reliably in this managed environment (truncated assembly / incomplete
object diagnostics); successful sanitizer coverage therefore uses NO_SQLITE.
SQLite remains compiled and exercised in ordinary AGG/Cairo/headless suites.

Проверки включают именованный и регистровый catch, вложенные повторные throw,
finally при return и замену результата, передачу исключения через функции,
нативные методы, GC ожидающего исключения и объекта return, закрытый мутекс в
файловой посылке, путь/URI/идентификатор, разрыв TCP после соединения, недоступный
адрес, устаревшую сессию, чужой reset и копию события отладчика.

Не заявляется завершение полной VM SWF8/JIT, всех стандартных Flash-объектов,
удалённого интерактивного отладчика, TLS, аренд ресурсов, восстановления после
аварии и согласования потерянного ответа set. Новый AS-файл является примером;
компилятор ActionScript в репозиторий не входит. Исполняются тесты байткода.

Generated logs and profiles remain outside tracked source; no validation logs
are committed. The mirror is not updated by this checkpoint: explicit command
is required. Source ZIP includes the internal Git history and version suffix.
