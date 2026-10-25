# Readable AS bridge runtime templates / Читаемые AS-заготовки

Edit `guards.as.in`, `module.as.in`, `proxy.as.in`; `@P@` is the generated identifier
prefix. These are the actual emitted AS bodies, not a second illustrative runtime.
Function comments explain parameters, completion, ownership and returns. Native
host marshaling and full Dis registration remain required.

```sh
python3 tools/embed_as_runtime.py
python3 tools/embed_as_runtime.py --check
```

Run from the workspace root. The C99 generator includes the checked-in embedding
`bindings_as_runtime.h`, so normal compiler builds acquire no Python dependency.
Do not hand-edit/reformat that generated header; preserve the source templates.
Frontend tests parse generated output through AS1/AS2/AS3 profiles and exercise
its scalar/dispatch/proxy contracts separately from actual native Dis integration.

Русский: это исходные тексты реально генерируемого AS-адаптера. Правьте их, затем
явно пересоздайте C-embedding. `@P@` заменяется проверенным префиксом. Наличие
адаптера не означает, что готов весь нативный мост или обработка произвольных типов.
