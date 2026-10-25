# Native string Actions — v0.357

[Русское описание](JIT_STRINGS.ru.md)

StringLength/MBStringLength, StringExtract/MBStringExtract,
CharToAscii/MBCharToAscii, AsciiToChar/MBAsciiToChar, StringAdd,
StringEquals, StringLess and StringGreater now lower to direct native helpers
calling the same counted/versioned services as interpretation. No interpreter
loop or Action decoding occurs inside these helpers.

Inputs stay in the invocation's rooted operand vector through conversion,
allocation and script callbacks. Results replace rooted inputs before removing
consumed operands. Extraction retains the service's count/start/text conversion
order. Concatenation and equal/less comparisons convert right before left;
greater converts left before right. Errors propagate through the existing safe
native boundary without committing a result.

The shared services' existing compatibility limits are preserved: SWF4–5
length/extract/character operations reject non-ASCII with [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48);
character construction maps zero to an empty string and surrogate units to a
replacement character. MB and ordinary variants share the interpreter policy.
This extension does not certify historical multibyte encodings or full Flash
string compatibility.

| Test | Coverage |
| --- | --- |
| `jit_strings` | Host/SWF4–8; interpretation, explicit/automatic native entries; all 12 Actions and missing operands; exact UTF-16 snapshots, embedded NUL, supplementary/lone surrogates, comparison order, character zero/replacement, returned text after code release and GC |
| `jit_strings_callbacks` | Collecting/throwing coercions for every service family; exact conversion order across all three execution modes |
| `jit_strings_allocation` | Host/SWF4/SWF8 allocation failures through all service families; restored native state and roots; retained output after code release and collection |

The automatic-fallback fixture now uses GetTime followed by Pop and constant 42,
because StringLength is supported. The time value is discarded, so the fixture's
assertion remains deterministic. Other Actions and all roadmap stages remain open;
foreign CPU/OS runtime acceptance is unchanged.

Linux Release: all 723 AGG, 723 Cairo and 704 minimal checks pass after restoring
generated ELF execute permissions. Three string tests and execution_modes pass
restricted heap/global ASan+UBSan; stack ASan and LeakSanitizer remain disabled.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
