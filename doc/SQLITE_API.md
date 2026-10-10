# Optional SQLite API (C89)

`flashvm/sqlite.h` exposes `fvm_sqlite_open`, `fvm_sqlite_exec`, `fvm_sqlite_query_int` and `fvm_sqlite_close`. It uses SQLite's official C API and is compiled only when the dependency is detected. It is not automatically accessible to ActionScript: register native callbacks explicitly and enforce file-path/SQL permissions in the host. See `examples/extending/sqlite`. SQLite API reference: https://sqlite.org/capi3ref.html
