# SQLite 3.50.4 amalgamation for FlashVM

Official amalgamation: sqlite3.c, sqlite3.h and sqlite3ext.h. See PROVENANCE.json
for the source URL, archive SHA-256 and public-domain provenance. The original
public-domain notice is retained at the start of the upstream source files.

SQLITE in DEFS.txt enables the bundled static target automatically, including
the existing FlashVM SQLite C adapter and examples. System SQLite is never
selected. NO_SQLITE omits the library, adapter and SQLite tests/examples.
SQLite is not installed as an ActionScript object by this checkpoint.

The profile uses SQLITE_THREADSAFE=1, SQLITE_OMIT_LOAD_EXTENSION=1, SQLITE_DQS=0
and SQLITE_DEFAULT_MEMSTATUS=0. Built-in databases, SQL, transactions and native
VFS remain available; runtime loading of arbitrary extension libraries is off.
The VM's ports/ policy does not rewrite third-party SQLite's own platform VFS.

flashvm.patch injects flashvm_sqlite_types.h into both the amalgamation and public
header and replaces LL integer literals with equivalent typed 32-bit halves.
LP64 uses standard C89 long; MSVC uses its native __int64; other 32-bit targets
need system int64_t/uint64_t. There is no truncation to a 32-bit SQLite ABI.
Only Linux x86_64 execution has been tested; the version is a compatibility pin.

SQLite включён по умолчанию и собирается из amalgamation без системной библиотеки.
Ключ NO_SQLITE полностью отключает адаптер. Сохранён 64-битный ABI; динамическая
загрузка расширений выключена. API ActionScript для SQLite здесь не добавляется.
