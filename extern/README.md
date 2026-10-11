# Bundled source dependencies / Внешние исходники

`mini_regex/` is an original, small C89 regex subset shipped as source and
built automatically by CMake. It is not PCRE, TRE, or a complete regex engine.

Other optional libraries (SQLite, Cairo, AGG, FFmpeg, SDL3, zlib) are **not
vendored yet**. Existing integration paths may use system-installed versions;
this does not satisfy a fully offline, source-vendored build. Before bundling
third-party code, upstream licenses, exact source revisions, patch sets,
platform build flags, and attribution must be recorded. Do not mistake
headers, binary packages, or download instructions for vendored source.

`mini_regex/` — собственная минимальная реализация подмножества регулярных
выражений на C89, автоматически собираемая CMake. Остальные опциональные
библиотеки пока не включены в `extern/` исходниками; для полностью автономной
сборки их ещё нужно добавить с учётом лицензий и платформенных настроек.
