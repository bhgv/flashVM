# Provider-first code fixture / Проверка приоритета кода

The [driver](../../test_code_order.py) compiles the same [module](dup.b) and
[dependency](dep.b) twice with source markers T/P, then runs the real dedicated
interpreter and JIT. The [native fixture](../../test_code_order.c) grants two
independent directories at `/code`: terminal first in the data stack, provider
first for code. A separate terminal alias primes the conflicting module cache.

Тест проверяет настоящий `load` и transitive dependency, не только свойства плана.
Чтение `payload.dis` через `Sys.open` остаётся обычными данными. После удаления
provider branch следующий load получает terminal image, а ранее загруженный
provider image сохраняется.

```sh
python3 disVM/runtime/test_code_order.py \
  build/native-dynamic/disvm \
  build/native-dynamic/bootstrap/limbo-bootstrap \
  disVM/upstream/module
```

Requires a built runtime, matching native-cell compiler and Python3. Expected:
`PASS provider-first code --no-jit` and `PASS provider-first code --jit`.
The driver owns temporary sources/images and its privately relinked executable;
normal runtime shutdown retires native directory bindings and module owners.

This is a local provider-execution test, not a network downloader or an SWF
decoder test. Future native-module formats and managed FlashVM transport remain
open. See [purpose/ownership contract](../../CODE-LOOKUP.md).
