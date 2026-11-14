# Исследование shared/copied namespace

`main.b` — исполняемый пример отладчика. Parent связывает source-a с visible,
запускает shared child и child с FORKNS/chdir. Оба удерживаются на first-chance
error. По stdin permit parent меняет visible на source-b, пока shared child
остаётся frozen; copied сохраняет свою группу/cwd. Второй permit завершает parent
после явного terminate children через debugger. [English](README.md),
[протокол и fields](../../runtime/NAMESPACE-DEBUG.ru.md).

Пример намеренно вызывает managed errors и требует встроенного debugger.
Внешних handlers нет: continue превратит ошибки в обычные unhandled errors
приложения. Здесь используется terminate. Debugger работает через отдельные
private pipes, не занимает stdin permits либо guest channels. Исходные frames/
groups живы до release; native cleanup/retirement снимает refs. Raw pointers
через границу не передаются.

Из корня после сборки console/bootstrap:

```sh
python3 disVM/runtime/test_namespace_debug.py build/disvm-opt/disvm build/disvm-opt/bootstrap/limbo-bootstrap disVM/upstream/module
```

Драйвер создаёт реальные каталоги, компилирует пример с bundled Sys/Processctl,
подключает debugger и проверяет interpreter/JIT и оба GC polls. Длинный путь с
Unicode/quotes собирается из128-byte chunks. Проверяются enumeration targets,
shared mutation, stale-revision refusal, copy isolation и normal retirement.
Тестовое исключение revision guard обязано вернуть настоящие новые bytes
`/source-b` по старой revision; crash/timeout/parser error не засчитываются.
Ожидаются четыре `PASS namespace debugger ...` и два
`DETECTED mixed-revision real source ...`.

| API | Назначение, владение |
| --- | --- |
| Sys.bind | Замена held binding; group держит resource ref. |
| Sys.pctl(FORKNS) | Копия refs перед private chdir. |
| Sys.chdir | Удержание cwd/import identity копии. |
| Sys.read / fildes | Заимствованный rooted stdin FD для permit. |
| Processctl.pid | Scalar PID через ready channel. |
| raise | Исходный first-chance frame жив до release. |
| namespace / namespace-read | Copy native state под mutex при точной revision. |
| terminate | Source owner делает unwind/retirement, без foreign suspension. |

Приёмка Linux x64 native/TSan; другие архитектуры требуют CI. Compiler/Python/
relink — средства разработки, не зависимости runtime.
[Удержание каталогов](../../runtime/DIRECTORY-ANCHORS.ru.md),
[отладчик](../../runtime/DEBUGGER.ru.md), [native API](../../runtime/vfs/observation.h).
Полный Chan/mount/provider и учёт memory bytes этим не заявляются.
