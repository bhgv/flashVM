# Native Inferno libraries / Библиотеки Inferno на C

Copyright (c) 2026 bhgv

This package imports **all of lib9 and tools/libstyx**, together with the required non-graphical headers,
legal notices and native Linux64 support, from Inferno's `experiment` branch.
[UPSTREAM.json](UPSTREAM.json) records the exact revision and every source hash.
The source files in `src/` are unchanged; fixes are made and tested in the owning
Inferno repository before vendor-sync imports them.

Пакет содержит **полные lib9 и tools/libstyx**, необходимые неграфические заголовки, лицензии и поддержку
Linux64 из ветки `experiment`. Исходники в `src/` сохраняются побайтно. Выбор
собираемых файлов берётся из оригинальных `lib9/mkfile` и `mkfile-Posix`: варианты
других ОС тоже сохранены, но одновременно с POSIX не компилируются.

## Build and use / Сборка и использование

```sh
cmake -S disVM/native -B build/inferno-native \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/inferno-native --parallel 4
ctest --test-dir build/inferno-native --output-on-failure
python tools/vendor-sync.py disVM/native
```

CMake targets `inferno9` and `infernostyx` produce `lib9.a` and `libstyx.a`.
Link server consumers with `infernostyx`; it carries lib9, pthread and math
requirements. Use the supplied target include directories, including the actual
native architecture's `lib9.h`, rather than the historical bootstrap ARM header.
`INFERNO_SOURCE` can select an owning upstream checkout for testing fixes before
importing them. No installed Inferno SDK or deployed Python runtime is needed.

Цели `inferno9` и `infernostyx` собирают исходные библиотеки, а не переписанные
кодеки. Для проверки upstream-исправлений можно явно задать `INFERNO_SOURCE`.
Python нужен только при конфигурации. Пакет сейчас рассчитан на Linux64;
на этой машине проверен amd64. Выбор arm64/riscv64 предусмотрен, но выполнение
на этих архитектурах этим набором результатов не подтверждено.

The native PC and hardware FP helpers already present in `experiment` are used.
The proven bootstrap host preparation scripts supply thread-local error state,
reentrant formatter/decimal support and native synchronization. These adapters
remain outside the exact source snapshot. They do not schedule guest processes.

Используются уже имеющиеся в `experiment` native64-функции PC/FP. Явные адаптеры
обеспечивают локальные для потока ошибки и синхронизацию форматтера/decimal-кода;
они не управляют исполнением гостевых потоков.

## Checked changes / Проверенные исправления

The imported upstream server fixes preserve 32-bit fids and 64-bit Qid helper
paths, avoid pointer narrowing in its existing invariant macro, retain a live
fid during same-fid walks and refuse replacement of another live fid. Read/write
callbacks receive a real native `ulong` temporary. Their returned counts are
checked before narrowing; read capacity is bounded by the response buffer, and
open modes are enforced. Partial and zero results are preserved without replay.
The public `styxqid(Path, int)` signature requires rebuilding callers.

Проверки исходного dispatcher используют настоящие C-кодеки и перехватывают
только отправку ответа. Они покрывают коллизии младших 16 бит fid, старшие биты
Qid, same-fid walk, режимы открытия, 64-битные смещения, нулевые/частичные
результаты и ошибочные native-счётчики больше запроса, включая значения выше
32 бит. Отдельно восемь native-потоков проверяют изоляцию ошибок и FP-состояния
и параллельное форматирование. Проверки работают и в release-сборке.

## Remaining integration / Оставшаяся интеграция

The complete libraries compile and are used by codec, original server-dispatch
and concurrent native-state acceptance. This is not yet a production native
VFS listener or `Sys.mount` client. The kernel-bound client in `emu/port/devmnt.c`
needs a separate Chan/Mnt/RPC host adapter. Do not link the new lib9 alongside the
bootstrap's other lib9 archive in one executable: unify those dependencies first.

Полная проверка C-сервера ещё не завершена. Следующий этап должен проверить и
исправить TCP-фрагментацию/частичную отправку, согласование msize и сброс сессии,
Tauth/attach и откат отказанного attach, границы stat/wstat, выдачу целых записей
каталога, живой каталог через callbacks и освобождение ресурсов при закрытии.
Текущие тесты не подтверждают готовность этих частей. Подготовленный корень
соединения должен оставаться живым обычным каталогом с независимыми правами;
сериализация или фиксация всего дерева не нужна.

See [native Styx integration](../runtime/STYX-NATIVE.md) and the
[live connection-root contract](../runtime/STYX-SERVING.md).

Original licenses remain in [src/NOTICE](src/NOTICE) and
[src/lib/legal](src/lib/legal); this import does not relicense Inferno.

## Graphics boundary / Граница графики

All graphics, including FreeType, belongs to FlashVM. Neither this native
package nor its library targets imports Inferno Draw/Memdraw/Tk/Prefab, FreeType,
PNG or SVG implementations/headers. The explicit header whitelist contains only
protocol, scalar runtime and FP declarations required by these libraries; complete
lib9/libstyx source directories remain intact. Floating-point text formatting
(`gfltconv`, `g_fmt`, `dtoa`) is numeric conversion, not a graphics operation.
DisVM application adapters issue rendering requests through FlashVM APIs.

Вся графика, включая FreeType, принадлежит FlashVM. В native-пакете отсутствуют
графические библиотеки и их заголовки. Полные исходники lib9/libstyx сохранены;
сопутствующие заголовки выбираются явно. Числовое форматирование не относится
к графике. Прикладные адаптеры DisVM обращаются к API рендеринга FlashVM.
