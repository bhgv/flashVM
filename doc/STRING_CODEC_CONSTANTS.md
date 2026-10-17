# String codec constants / Константы строкового кодека — v0.266

The internal WTF-8/UTF-16 bridge now uses documented uppercase macros for
sequence sizes, surrogate ranges, supplementary offsets, payload widths/masks
and continuation-byte bounds. The terminated borrowed-string size marker is
named [`FVM_STRING_TERMINATED_SIZE`](../src/core/string_constants.h#L6) in `src/core/string_constants.h`. Unicode
representation constants live in `src/core/unicode_constants.h`.

No representation, wire format, Unicode mapping or decoding policy changes.
The public scalar UTF-8 codec remains strict; the internal WTF-8 bridge preserves
lone UTF-16 surrogates. Supplementary scalars still expand to two units and
counted strings retain embedded NUL. The change also documents what each internal
codec helper reads, returns or appends. Ordinary array positions remain indices.

В общем кодеке вместо чисел используются макросы с комментариями: длины
последовательностей, границы суррогатов, смещения, маски и размеры полезных полей.
Маркер строки с завершающим NUL вынесен в отдельный `.h`. Форматы строк и канала
не изменены. Это улучшение читаемости существующего поведения; оно не закрывает
оставшуюся семантику строк, исторические кодировки или стадии роудмапа.

Validation: GCC13.3 Linux AGG/Cairo each pass 508 tests; six AGG executables
required execute-bit restoration and a successful rerun. All 26 external SWF<=8
traces match exactly in both profiles. The 29 selected UTF-16/string/counted-name
ASan/UBSan fixtures pass with heap/global coverage, without stack/leak checks.

## v0.275 — canonical names and UTF-16 ordering

Property-name canonicalization and allocation-free string ordering now share
the documented surrogate bounds, supplementary offset and payload width/mask.
Equal numeric values retain encoding-specific names. Lone surrogates, embedded
NUL and supplementary pair equivalence retain their previous representation.
Нормализация имён и сравнение строк используют общие Unicode-макросы; алгоритмы
и форматы не изменены.

Linux AGG/Cairo each pass 526 tests. Four AGG binaries needed execute-bit repair
and rerun. An incomplete Cairo log was rejected and a fresh full run passed.
Four selected heap/global ASan+UBSan checks pass (no stack/leak coverage); all
26 original SWFs match in each profile. No roadmap stage is completed.

## v0.276 — builtin String bounds and callback IDs

String encoding/casing now uses the same UTF-16 pair constants as property keys.
Numeric conversions name their Uint32 modulus/mask and UTF-16 unit modulus.
Unicode host casing names capital/final sigma; loaded Flash casing still uses
its existing versioned table. `src/standard/string_constants.h` documents the
existing split allocation bound and stable serialized callback identities.
Nothing renumbers callbacks or changes split policy. Числовые параметры строк
заменены смысловыми макросами; форматы и ограничения остаются прежними.

Linux AGG/Cairo each pass 526 tests after execute-bit repair and successful
reruns for 1/8 binaries respectively. Six affected heap/global ASan+UBSan checks
pass (no stack/leak coverage). All 26 original SWFs match in both profiles.
The remaining standard library and roadmap acceptance gates stay open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_STRING_TERMINATED_SIZE` | macro / define | [src/core/string_constants.h:6](../src/core/string_constants.h#L6) |
<!-- source-links:end -->
