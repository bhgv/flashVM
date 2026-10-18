# SWF linker

Version 0.1.0. [Русский](README.ru.md).

Reserved independent C99 project. No linker implementation, object format or
public API is supplied yet. `cmake -S . -B build` configures the project boundary;
`cmake --build build` currently has no source targets. It does not create SWF files.
The future linker will consume explicit compiler artifacts and must not require
FlashVM source or workspace-relative sibling paths. Keep the shared MIT LICENSE
with separated distributions and preserve upstream notices.
