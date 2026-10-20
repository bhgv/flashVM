# Explicit SDK/toolchain files

[Русский](README.ru.md). [Complete guide](../BUILDING.md),
[evidence and limitations](VALIDATION.md).

[explicit.cmake](explicit.cmake) takes absolute compiler/SDK paths, preserves
settings in try_compile and separates host tools from target libraries. Copy it
explicitly with a detached project; it needs no sibling sources. Use the NDK
toolchain itself for Android.

Template version: 0.1.0. C-only projects need no C++ SDK; projects enabling C++
require FVM_TARGET_CXX. Missing it produces FVM_TARGET_CXX_REQUIRED instead of
silently selecting the host g++.
