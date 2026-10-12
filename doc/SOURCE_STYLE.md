# Source presentation checkpoint — v0.19 / Оформление исходников — v0.19

## English

This change documents and formats the 225 first-party C, C++, ActionScript and
header files in `src/`, `include/`, `ports/` and `examples/`. It adds file purpose
comments, function explanations, header type/member/constant documentation and
separate initialization, computation, output and cleanup sections. Required
execution and error-handling order is preserved.

The examples are tutorials: each nonempty code line has an explanation, including
API ownership, checks, control-flow boundaries and multi-line arguments. Existing
notes identifying ActionScript design targets or unsupported syntax remain intact.
Formatting does not add an ActionScript source compiler or implement those targets.

Search for `[GC:]` to find collector and lifetime annotations. They cover the
collector passes, finalization and resurrection, root scopes, AVM1 stack/register
roots, native callback captures/barriers, allocation accounting and related shared
resource lifetime code. Header comments explain the state retained by the collector.

`external/`, `tests/`, `scripts/`, `cmake/` and all build scripts are unchanged.
No runtime feature, public ABI, wire format or dependency version is changed.
The active source tree remains the committed v0.18 implementation with revised
presentation; unfinished implementation work remains separate from this change.

Verification on Linux with GCC/G++ 13.3:

| Configuration | Passed checks |
| --- | ---: |
| Headless, bundled SQLite and OS adapters | 167 |
| AGG, fonts, image codecs and bundled libraries | 177 |
| Cairo, fonts, image codecs and bundled libraries | 177 |
| Minimal portable configuration | 158 |
| Strict C89/C++98, heap/global ASan and UBSan | 164 |

Several binaries lost executable permissions in the managed workspace. The affected
launches were repeated after restoring permissions: one AGG test, two Cairo tests,
and the three remaining sanitizer process checks. They passed; no failing runtime
assertion was hidden by these retries. The initial sanitizer driver stopped on a
permission error; its remaining checks were run explicitly against the same binaries.
ASan stack checks and LeakSanitizer are disabled because this environment cannot
provide the stack/process information required by its sanitizer runtime. The
allocation-failure container test uses UBSan only. SQLite is disabled in this
sanitizer profile. These are existing environment constraints, not full sanitizer
coverage or validation on Windows, macOS or Android.

An independent structural audit found explanations before all 1,496 parsed C/C++
functions/prototypes, comments for header data members, and explanations for all
1,529 nonempty tutorial code lines. All directly identified GC lines had `[GC:]`
annotations. Executable-token comparison against v0.18, after C line splicing and
removing comments/whitespace, matched in all 225 files. Macro names and function-like
macro signatures also matched. These checks complement compilation and execution;
they do not certify complete SWF 8 support or the completeness of the roadmap.

Generated logs and audit results are delivered separately, outside Git.

## Русский

Оформлены 225 собственных файлов C/C++/ActionScript и заголовков в `src/`,
`include/`, `ports/`, `examples/`. Добавлены описания файлов и функций, пояснения
типов, полей и констант в заголовках. Инициализация, вычисления, вывод и освобождение
ресурсов выделены в логические блоки с сохранением необходимого порядка операций
и обработки ошибок.

В примерах объяснена каждая непустая строка кода: владение ресурсами, проверки,
границы блоков и продолжения аргументов. Прежние пометки о проектируемых,
неподдержанных конструкциях ActionScript сохранены. Оформление не добавляет
компилятор исходного ActionScript и не реализует эти проектируемые возможности.

Префикс `[GC:]` позволяет искать строки сборщика и управления временем жизни:
маркировку, финализацию и воскрешение объектов, временные корни, стеки и регистры
AVM1, захваты нативных обработчиков, защиту от повторного входа, учёт выделений
и связанные участки управления общими ресурсами. Поля заголовков объясняют,
какое состояние удерживается сборщиком.

`external/`, `tests/`, `scripts/`, `cmake/` и скрипты сборки не изменены.
Поведение VM, публичный ABI, формат передачи и версии библиотек не менялись.
Незавершённая реализация новых возможностей отделена от этой правки оформления.

Результаты сборок и тестов приведены в таблице выше. Проблемы запуска отдельных
бинарников из-за прав доступа устранены восстановлением прав и повторным запуском
затронутых проверок. Для sanitizer-профиля оставшиеся три межпроцессные проверки
выполнены отдельно на уже собранных бинарниках после остановки драйвера из-за
ошибки доступа. Проверено суммарно 164 пункта этого профиля. Ограничения:
ASan для кучи/глобальных данных и UBSan включены, проверки стека ASan и
LeakSanitizer отключены из-за ограничений среды; тест исключений контейнеров
работает только с UBSan, SQLite в этом профиле отключён. Это проверка Linux,
а не подтверждение работоспособности на остальных платформах.

Структурная проверка не нашла пропусков перед 1 496 функциями/прототипами C/C++,
в пояснениях полей заголовков и 1 529 непустых строках примеров. Все непосредственно
выявленные строки GC снабжены `[GC:]`. Исполняемые токены всех 225 файлов совпали
с v0.18 после обработки продолжений строк C и удаления комментариев/пробелов;
сигнатуры макросов также сохранены. Сборки и тесты дополняют эту проверку, но
не означают завершения SWF 8 VM или этапов принятого плана.

Технические результаты и логи предоставляются отдельным архивом и не входят в Git.
