# Pixman 0.38.4 for bundled Cairo

Pinned official release and archive hash: PROVENANCE.json. The library sources
and upstream COPYING are included. The CMake profile uses the scalar core only:
no assembler, SIMD flags, runtime CPU-specific implementation or external tools.
Thread-local state uses pthread keys on POSIX and upstream Win32 TLS on Windows;
thread safety is not disabled. Native fixed-width system integer types are needed.

flashvm.patch records the C89 changes: 64-bit constants built from unsigned
32-bit halves, a portable diagnostic function label, an endif comment, the
thread-local declaration's redundant semicolon, and sqrtf replaced by an explicit
float conversion around C89 sqrt. Build/test coverage is Linux x86_64.

Pixman нужен Cairo и собирается автоматически из этой папки. SIMD отключён;
поддержка нескольких потоков сохранена. Происхождение, лицензия и патч сохранены.
