# String codec constants / Константы строкового кодека — v0.266

The internal WTF-8/UTF-16 bridge now uses documented uppercase macros for
sequence sizes, surrogate ranges, supplementary offsets, payload widths/masks
and continuation-byte bounds. The terminated borrowed-string size marker is
named `FVM_STRING_TERMINATED_SIZE` in `src/core/string_constants.h`. Unicode
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
