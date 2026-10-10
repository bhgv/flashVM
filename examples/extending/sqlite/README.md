# Extending: SQLite (optional adapter implemented)

This C89 example creates an in-memory SQLite database and queries it. It is built when SQLite development files are available. The `.as` script describes a **future host-registered function**, not a built-in AVM1 API. Never expose unrestricted SQL or arbitrary file paths to untrusted SWF; prefer host-authorized databases and parameterized statements. The adapter currently offers a basic `exec` and integer query, not prepared-statement binding.
