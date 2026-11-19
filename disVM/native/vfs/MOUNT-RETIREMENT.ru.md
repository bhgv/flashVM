# Deep mount fixture: stop request и освобождение открытых fid

Copyright (c) 2026 bhgv

[English](MOUNT-RETIREMENT.md) · [Нативные библиотеки](../README.md) ·
[Интеграция юнионов и модулей](../../runtime/VFS-TRAVERSAL.md)

## Проблема и новое поведение

Раньше deep native mount fixture выставлял флаг выхода main-loop прямо в
callback Twrite команды `stop`. После Rwrite у клиента ещё были открыты file и
ctl, и клиент выполнял обычный Tclunk. Fixture мог выйти из dispatch loop и
закрыть соединение раньше завершения этих сообщений. Relay правильно отвергал
неполный header; игнорирование ошибки или повтор эффекта скрыли бы дефект порядка.

Теперь `stop` отмечает **запрос завершения**, а не немедленный teardown. Единственный
клиент может читать открытый файл и получать ответы Tclunk. При disconnect
исходная libstyx освобождает оставшиеся fid и вызывает freeclient; только этот
callback выбирает выход из main-loop. Затем main вызывает styxend вне callback.
Это single-client test fixture, не новый production shutdown protocol.

Изменены только fixture и regression driver. Исполнение DisVM, выбор VFS,
C library API, canonical Inferno snapshots и 9P messages остаются прежними.
Нет VM pause, guest cooperation, увеличения timeout и ослабления frame checks.

## Владение и последовательность

1. Клиент проходит шестнадцать компонентов каталога, затем оставшийся каталог
   и файл. Освобождение intermediate unopened fid отделено от opened fid.
2. File открывается readonly, ctl — write-only. В конце требуется ровно два
   opened-handle close и один final client retirement.
3. Одна Twrite команды `stop` по offset0 подтверждает ровно четыре байта.
   Callback не сохраняет input и не лишает доступа к opened objects.
4. Следующий настоящий Tread по прежнему file fid обязан вернуть `depth`.
   Это детерминированно обнаруживает premature teardown без timing планировщика.
5. В режиме `clunk` оба opened fid получают явное Tclunk acknowledgement.
   В режиме `disconnect` file закрывается Tclunk, а EOF соединения освобождает ctl.
6. Исходная freeclient удаляет fid до вызова callback fixture. Callback только
   учитывает retirement и выбирает выход; рекурсивного styxend внутри него нет.

Driver намеренно делит request prefixes, проверяет response size/type/tag/count
и ограничивает startup/socket/child waits. Transport EOF не считается успешным
Tclunk. При ошибке unfinished fixture уничтожается и reap выполняется обязательно.
Ни один режим не повторяет неуспешную операцию.

## Исходники

- [C-сервер и callbacks](../tests/mount/deep.c): writefile выставляет
  stop_requested; freeclient выбирает stopping после fid retirement.
- [Независимый 9P regression driver](../tests/mount/retirement.py): exact replies,
  retained read, explicit clunk/disconnect и итоговые lifetime counters.
- [Существующий native mount client](../tests/mount/client.c) и
  [строгий fault relay](../tests/mount/wire.py): поведение и проверки не ослаблены.
- [Регистрация CTest](CMakeLists.txt): два новых bounded retirement cases.

Все изменения implementation/test server — на C; независимый packet driver —
на Python, что разрешено для примеров и тестов.

## Воспроизведение

Из корня репозитория, с явно выбранными текущими закреплёнными исходниками:

```sh
cmake -S disVM/native -B build/mount-retirement \
  -DINFERNO_SOURCE="$PWD/disVM/native/src" \
  -DINFERNO_HOST_SUPPORT="$PWD/tools/limbo-bootstrap" \
  -DINFERNO_VFS_SUPPORT="$PWD"
cmake --build build/mount-retirement --target \
  vfs-mount-deep-server vfs-mount-test server-loopback-test \
  vfs-file-wire-test vfs-peer-wire-test vfs-mount-control vfs-event-resource-server -j4
ctest --test-dir build/mount-retirement \
  -R '^(vfs-mount-|vfs-mux-provider-)' -j4 --output-on-failure
ctest --test-dir build/mount-retirement \
  -R '^(vfs-mount-retirement-|vfs-mount-deep$|vfs-mux-provider-deep$)' \
  -j4 --repeat until-fail:30 --output-on-failure
```

Первая выборка — 36 mount/mux/control cases, не весь SDK. Повторение запускает
каждый из четырёх выбранных cases 30 раз с остановкой на первом отказе; это не
retry-until-pass. Сборка всех перечисленных targets позволяет не путать отсутствующие
бинарники в старом build-каталоге с protocol failure.

Для instrumentation используйте другой build-каталог и
`-DCMAKE_C_FLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all'`,
`-DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'`, затем запустите CTest с
`ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`. Leak scanning в sandbox
недоступен: это проверка ASan/UBSan, не приёмка LSan.

## Проверка и оставшиеся границы

Linux amd64: прежний fixture отказывает на post-stop read с Broken pipe,
connection reset или ранним EOF. Исправленный проходит оба direct modes,
36/36 mount/mux/control cases в обычной и ASan/UBSan сборках, а также
120 повторных concurrent executions.

Прежняя intermittent deep relay error согласуется с воспроизведённым shutdown
defect. Это не доказывает, что он был единственной возможной причиной всех
исторических transport failures. Grant checkpoint variants7/11 остаются
отдельным расследованием; успешный полный parallel SDK здесь не заявлен.
