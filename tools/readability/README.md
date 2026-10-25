# Source readability / Читаемое оформление исходников

The user's current rules are recorded in [root AGENTS.md](../../AGENTS.md).
They apply to handwritten code, including tests and examples. The common native
profile is [.clang-format](../../.clang-format); component copies allow detached
FlashVM, compiler, FIR and linker checkouts to retain the same layout.

Правила: обязательные `{ }` у тел ветвей и циклов; один самостоятельный оператор
на строку; четыре пробела для отступа. После группы include и перед `public:`,
`protected:`, `private:` оставляется пустая строка. Перед комментарием-описанием
функции, метода, структуры, enum или класса — **две пустые строки**. Разделители
заголовка `for` являются частью синтаксиса, а не отдельными операторами тела.

Before each function, explain its purpose, every parameter and return value.
State ownership, failure and blocking/thread constraints where relevant. Explain
globals, defines, enum variants and fields in aligned trailing comments. Avoid
filler such as “perform function” and never invent ownership or runtime guarantees.
Use explicit `if` checks with an error message or an appropriate failure/recovery
action. Do not use the assertion macro or optimized-away Python assertion statements.
Function-pointer typedefs describe the callback contract, not a native wire ABI.

Перед функцией нужен содержательный комментарий: назначение, каждый параметр,
возврат, владение, ошибки и ограничения потоков/блокировки. Глобальные переменные,
макросы, варианты enum и поля снабжаются пояснениями справа от объявления;
комментарии выравниваются в группе. Смысл комментариев проверяется по реализации,
а не по имени функции. Объявления разных полей размещаются на разных строках.

## Reproduce the native layout / Повторить оформление C/C++

Use clang-format **18.x**. It is a development utility, not a build prerequisite.
No formatter is downloaded or run by a normal project build.

```sh
python3 tools/readability/format_native.py --formatter clang-format --write
python3 tools/readability/format_native.py --formatter clang-format
python3 tools/readability/check_semantics.py --formatter clang-format
python3 tools/readability/check_explicit_errors.py
```

The default is a read-only drift check and returns nonzero when tracked files need
formatting. `--write` edits the selected files. `--jobs N` controls formatter
parallelism. The driver checks every tracked handwritten `.c`, `.cpp`, `.cc`, `.h`,
`.hpp` and `.mm`, including tests/examples. It excludes path components named
`upstream`, `external` and `generated`. No untracked source is silently swept in.
Stage a new source explicitly before using the repository-wide check.

[format_native.py](format_native.py) uses the profile and restores two blank lines
before recognized declaration/definition documentation. It rejects changes to
non-comment tokens other than braces and macro line continuations; string/character
literal contents remain significant. Braces are allowed because the formatter
adds controlled bodies. This lexical guard is **not** a proof of semantic
preservation: compiler/runtime checks remain necessary, especially around macros,
conditional compilation and dangling `else`. The formatter does not invent
parameter descriptions or certify comment coverage.

Проверка без `--write` не меняет файлы. Токены строковых/символьных литералов,
имена, числа и операторы должны сохраняться; добавление скобок проверяется также
компиляцией и регрессиями. Подключаемые сторонние исходники, снимки Inferno/Coco/R,
генерируемые парсеры и сырые эталонные fixtures не переписываются. Их изменения
должны сохранять происхождение и идти отдельной работой в соответствующем проекте.

## v0.1539 pass / Проход v0.1539

- Native layout: repository-wide C/C++ pass, including platform adapters and tests.
- Python: Black 24.10.0 with a 100-column limit and its AST-equivalence safety check;
  opaque source strings inside tests are preserved.
- Limbo: braced controlled bodies and separate statement lines in the handwritten
  `.b` sources; IDE interface fields/constants and service function contracts reviewed.
- Expanded first-error/input/ownership contracts in the four AS1/AS2/AS3/Limbo
  frontend C APIs and the native GC worker service.
- Full semantic-comment review of every legacy function/member is **not** certified
  by this pass. Existing comments outside the reviewed contracts retain their
  previous scope. AS/Coco-R grammar layout requires its own
  language-aware pass; native formatting must not be applied to it blindly.
- Shell scripts are outside this source-formatting patch.
- Former native/Python assertion statements are explicit conditions with failure
  handling. Native test failures print file/line and exit with EXIT_FAILURE; required
  calls still execute. Python failures explicitly raise their previous error class
  and are no longer disabled by Python -O. Vendored fixtures retain upstream code.

Полный смысловой аудит комментариев всех старых функций и полей этим проходом
не объявляется завершённым. Массовое оформление не равно выполненной документации
параметров каждого исторического API. Правило обязательно для последующей работы;
форматтер не заменяет такой аудит и не меняет статус реализации VM или языков.

The public C89 / implementation C++98 FlashVM and C99 frontend/host contracts stay
unchanged. This pass does not move `ports`/`external` or replace either collector.
See [roadmap](../../flashVM/doc/ROADMAP.md), [Dis host](../../disVM/host/README.md)
and [GC comparison](../../docs/GC-COMPARISON.md) for implementation limits.
