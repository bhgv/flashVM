# Explicit SDK/toolchain files

[Русский](README.ru.md). [Complete guide](../BUILDING.md),
[evidence and limitations](VALIDATION.md).

[explicit.cmake](explicit.cmake) takes absolute compiler/SDK paths, preserves
settings in try_compile and separates host tools from target libraries. Copy it
explicitly with a detached project; it needs no sibling sources. Use the NDK
toolchain itself for Android.
