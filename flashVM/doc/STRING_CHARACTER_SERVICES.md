# String/character services / Строки и символы — v0.265

[`fvm_exec_string_length`](../include/flashvm/execution.h#L192), [`fvm_exec_char_code`](../include/flashvm/execution.h#L194) and [`fvm_exec_char_from_code`](../include/flashvm/execution.h#L196)
provide C89 value interfaces for the ordinary/multibyte action pairs. Each
operation has its own handler file, covering the supported SWF versions without
interpreter stacks, register banks or bytecode access. StringExtract retains its
separate three-operand service.

This decomposition preserves the supported behavior: host/SWF6–8 length counts
UTF-16 units including NUL; character reads inspect the first unit, substituting
U+FFFD for surrogate units. Numeric character construction wraps to Uint16, maps
zero to empty, and substitutes U+FFFD for surrogate units. Loaded SWF4–5 remains
limited to ASCII and returns unsupported for non-ASCII inputs. In particular,
the real historical byte/locale differences between ordinary and multibyte
operations remain open; sharing the supported subset does not certify them.

[GC:] Public services retain their input during coercion callbacks and preserve
the result on validation errors, exceptions, unsupported input and allocation
failures. Aliases are allowed. Character construction reuses the common signed
Int32 wrapping service before extracting Uint16, removing the duplicate numeric
wrapping helper from the interpreter.

Documented macros name unary/extraction root layouts, one-based substring origin,
operand layouts, surrogate/replacement bounds and the ASCII boundary. Fixture
bytes and independent oracle data remain numeric deliberately.

`execution_string_characters` covers C89, unloaded/SWF4–8, embedded NUL, surrogate
pairs, Uint16 wrapping, aliases, coercion callback GC and exceptions.
`avm1_string_characters` exercises all six ordinary/multibyte action codes.
`execution_string_characters_allocation` checks each new service's allocation
boundaries and root/output restoration.

Три операции вынесены из основного цикла в отдельные файлы с общим C-интерфейсом.
Их можно вызвать из подготовленного нативного тела, не зная формат p-кода.
Значения удерживаются во время преобразований с callback и GC; результат не
изменяется при ошибке. Смысловые числовые константы заменены макросами в `.h`.
Исторические кодировки SWF4–5, полный AVM1 и машинный JIT ещё не завершены.

Primary reference for the supported modern unit/replacement behavior: pinned
Ruffle [`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs),
`action_string_length`, `action_char_to_ascii`, `action_ascii_to_char` and their
multibyte counterparts. That source also records unresolved legacy encoding
differences; it is not evidence of full old-Player conformance.

Validation: Linux GCC13.3 AGG/Cairo each pass 508 tests, with successful reruns
after restoring execute bits lost by this environment. The selected five-test
ASan/UBSan run passes (heap/global only; no stack or leak checks). All 26 original
SWF<=8 traces match in both profiles. The excluded string_ops_swf6 file has an
unmodified CWS/SWF15 header, size 1189 and SHA256
4736047901af80560705ae16048eafb0a7b89faa22f8d9a5003f1be1ac1c97bc;
it supplies no SWF<=8 acceptance evidence.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_char_code` | function | [include/flashvm/execution.h:194](../include/flashvm/execution.h#L194) |
| `fvm_exec_char_from_code` | function | [include/flashvm/execution.h:196](../include/flashvm/execution.h#L196) |
| `fvm_exec_string_length` | function | [include/flashvm/execution.h:192](../include/flashvm/execution.h#L192) |
<!-- source-links:end -->
