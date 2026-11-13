# Inferno network tuple boundary

Copyright (c) 2026 bhgv

Прочитан [INFERNO-0163](https://github.com/bhgv/Inferno-OS-bhgv/blob/82814edfbc0d68d495ce5a055fa9b23b734eac9b/doc/dis64-inferno-reply-0172.md).
Это ответ на наш FLASHVM-0172, а не дополнительная готовность DisVM networking.
Source pins остаются `7f224145`: последующий `82814edf` меняет только документы.

## Проверенные другой стороной исходные данные

Для generated native64 **Word=4/ref=8** заголовка Sys:

| Функция | Frame bytes | Managed arguments от byte 64 | Frame bitmap |
| --- | --- | --- | --- |
| dial | 80 | String 64, String 72 | `00 c0` |
| announce | 72 | String 64 | `00 80` |
| listen | 88 | FD 64, FD 72, String 80 | `00 e0` |

Caller-result address находится на offset 32; он не является GC root аргументов.
`Connection` — value ADT: два `ref FD` и String, 24 bytes, bitmap `e0`.
Результат `(int, Connection)` занимает 32 bytes: Word status на 0, padding 4..7,
три managed поля на 8/16/24, отдельная result bitmap `70`. Получать offsets нужно
из generated layout. Эти данные не разрешают импорт нового DIS64 в hosted NC64.

Inferno сообщил AMD64 generated-layout probe и 4102 extracted C-wrapper
adoption/pipe/connection/failure cases под ASan/UBSan для emu WORD32. Это его
проверки, не наш независимый replay и не acceptance concurrent DisVM/сети.

Существующий C wrapper строит новый Connection до замены прежнего managed
payload. После native-вызова status уже может стать -1; при construction failure
старый managed payload сохраняется. Поэтому весь tuple не является одной
атомарной транзакцией 32 bytes. Native failure очищает старый Connection;
успех заменяет owners, затем ставит status 0. Conversion failure до native вызова
требует отдельного unwind пути.

## Что предстоит состыковать

Ни текущие 22 native boot exports, ни 14 private file Sys signatures не включают
dial/announce/listen. Binder FD96/Word104 не принимает произвольный managed tuple.
Inferno предлагает отдельный tuple-result gate: формы аргументов/результата,
caller roots, generated entry identity, admission до allocation и частичный
rollback; затем настоящий owning loader/console путь. Public tuple kind не назначен.

Наш [native session/stream слой](README.md#реализованная-native-основа) покрывает
владение/admission и bytes. Для DisVM adapter всё ещё нужны rooted managed targets,
типизированный вызов, ref routing, result cache/ownership, transport authentication
и invocation cycle policy. Sys FD остаётся локальным владельцем дескриптора.

Выбранное для private foundation правило: Retain/Release эффекты проходят тот же
exact request-ID admission, что Invoke/Set; прямые C retain/release доступны только
trusted adapter. Duplicates не повторяют effect, stale ID не получает нового смысла.
Отдельная регрессия проверяет Release replay после collect и reuse slot: root
новой generation остаётся жив. Это ещё не сетевой authenticated capability protocol.
