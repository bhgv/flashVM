# Authoring debug tags / Теги отладки автора SWF

v0.235 validates EnableDebugger (58), DebugID (63) and EnableDebugger2 (64)
as bounded root-document data. EnableDebugger2 includes a reserved UI16 before
a terminated password/hash byte string; DebugID has exactly 16 opaque bytes.
The root timeline retains the tag kind and opaque payloads as managed byte
objects, preserving them through GC and graph transfer. Playback and existing
execution limits remain controlled by the embedding configuration.

В v0.235 загрузчик проверяет границы служебных тегов отладки и сохраняет их
содержимое в управляемом графе. Пароль/хеш и идентификатор хранятся как байты,
включая значения вне UTF-8; GC и передача через канал сохраняют их без перекодировки.
Это поддержка входного формата, а не завершение отладчика из пункта 7.

Tag identifiers, fixed fields and FileAttributes masks are documented uppercase
macros in `src/swf/format_constants.h`. Scalar UTF-8 boundaries/masks used for
Metadata are in `src/core/unicode_constants.h`; surrogate code units remain
invalid for Metadata even though runtime strings support WTF-8.

The unchanged pinned Ruffle closure_scope SWF7 now matches all seven trace lines,
with zero exit and empty stderr. The original selected corpus reaches 23/23.
Higher-version With/arguments fixtures remain excluded rather than having their
headers rewritten. The new swf_debug_document test exercises both debugger tag
layouts, arbitrary bytes, collection, graph transfer, wrong DebugID sizes and
unterminated password data. Primary layout reference: pinned Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd, `swf/src/read.rs` tag dispatch and
`read_debug_id`. All nine roadmap stages remain open.

Validation: Linux AGG and Cairo Release each pass 437 tests, and each produces
byte-exact traces for all 23 original SWFs. AGG required nine execute-bit launch
retries; these passed after restoring executable permissions. Seven focused
heap/global ASan+UBSan checks pass; stack and leak instrumentation are disabled.
New public/scoped macro headers compile with strict C89. Windows, macOS and
Android execution and all JIT backends remain unverified/unimplemented.
