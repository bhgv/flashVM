# Живые переменные модулей в паре DisVM и FlashVM

[English](STYX-MODULE-REFS.md) · [Интерфейс](../module/styxmodref.m) · [Реализация](../limbo/styxmodref.b) · [C SDK](../../lib/styxrpc/reference/module_ref_port.ru.md)

Программа в смонтированной паре может передать живой знаковый Int32
любого владельца через именованный канал. `Styxmodref` возвращает
типизированный `ref Cell` в Limbo. FlashVM использует прозрачные
ссылки ActionScript. Ссылка сохраняет идентичность экспорта и режим доступа.

## Экспорт владельца FlashVM

Подключите в программе Limbo `sys.m` и `styxnamed.m`.
`styxnamed.m` включает объявления ссылок на переменные модулей.
Загрузите соответствующие `styxnamed.dis` и `styxmodref.dis` и
инициализируйте их перед открытием канала. Штатная сборка DisVM
включает оба модуля. Соединение предоставляет обычный запуск через
`--styx-mount-fd`, `--styx-mount-tcp` или `--styx-local-pair`.

После публикации `counter` с типом Int32 в `ref-pair.dis` программа
Limbo захватывает файл с правом записи и передаёт его дескриптор
через обычный именованный канал.
Путь отсчитывается от исходного подключения и не зависит от рабочего
каталога Limbo или имени маунта в пространстве гостя.

```limbo
pipe := named->open("/module-refs");
cell := refs->capture("mounted/ref-pair.dis/.var/counter", 1);
pipe.send(cell.payload());
returned := refs->adopt(pipe.recv());
pipe.close();
after := returned.read();
returned.write(42);
returned.close();
cell.close();
```

Фильм FlashVM подключается к тому же каналу после захвата. Он
получает ссылку на свой экспорт, заменяет значение и возвращает
ту же ссылку. После возврата из кадра плеер может обслужить чтение из Limbo.
В примере Limbo переменная `after` получает `-73`, а последняя запись
устанавливает исходный счётчик в `42`.

```actionscript
var pipe = vfs.chan("/module-refs");
var alias = pipe.recv();
alias = 0 - 73;
var sent = pipe.send(ref alias);
var closed = pipe.delete();
```

Режим `0` создаёт ссылку только для чтения. Передавайте её через тот же
`payload()`, режим сохраняется. `adopt` проверяет область подключения и
откладывает открытие экспорта до первой операции со значением. При вызовах
`capture`, `read` и `write` владелец должен обслуживать запросы VFS.
Отзыв или замена публикации делают её ссылки недействительными, включая
принятый дескриптор, который ещё не открывал fid. Ссылку можно использовать
после закрытия канала. Выполняйте операции с одной ячейкой последовательно.

## Экспорт владельца DisVM

Вызовите `nodes->publishat("/mounted/counter.dis/.var/value", "rw", big 4)`
из `Styxnodes`, когда родительские каталоги готовы. Для экспорта только
на чтение используйте `"r"`. Владелец обслуживает четыре байта со
смещения ноль, старший байт знакового Int32 идёт первым. Если основная
программа читает или пишет через возвращённую ссылку, обслуживайте запросы
в отдельном потоке Limbo.

После публикации захватите местный экспорт из Limbo. Путь отсчитывается
от исходного экспортированного корня. Типизированный приём возвращает
`Message.Modref` с принятой ячейкой. При первом обращении к её значению
проверяются путь и Qid экспорта.

```limbo
pipe := named->open("/dis-module-refs");
cell := refs->capturelocal("mounted/counter.dis/.var/value", 1);
pipe.sendvalue(ref Message.Modref(cell));
pick returned := pipe.recvvalue() {
Modref =>
    updated := returned.reference.read();
    returned.reference.close();
}
cell.close();
pipe.close();
```

```actionscript
var pipe = vfs.chan("/dis-module-refs");
var alias = pipe.recv();
alias = 42;
var sent = pipe.send(ref alias);
var closed = pipe.delete();
```

У замены под тем же именем другой Qid. Отзыв закрывает доступ к
публикации через открытые и ещё не открытые ссылки. Закрытие fid владельца
может завершаться асинхронно. Перед повторной публикацией дождитесь
исчезновения старой записи. FlashVM также может открыть удалённый экспорт
DisVM через `vfs.intVariable()` и передать `ref proxy.value`.

## Проверка

Запустите [команду парной проверки](STYX-CHANNEL.ru.md#проверка) с `--modref`.
Она собирает [программу Limbo](../../lib/styxrpc/tests/flashvm/modref_pair.b)
и [фильм ActionScript](../../lib/styxrpc/tests/flashvm/modref_pair.as)
компиляторами и линкером проекта. Ожидается
`PASS native modref RPC paired VMs`. Тест пишет через обе VM, сверяет
область и права, затем заменяет публикацию под тем же именем.
Добавьте `--tcp` для прямого TCP и `--jit` для JIT.

Для экспорта DisVM используйте `--modref-reverse`. [Владелец Limbo](../../lib/styxrpc/tests/flashvm/modref-reverse_pair.b)
и [фильм AS](../../lib/styxrpc/tests/flashvm/modref-reverse_pair.as)
обмениваются ссылками с правом записи и только для чтения, проверяют обе
границы Int32 и новый Qid после отзыва. Ожидается
`PASS native modref-reverse RPC paired VMs`.

## Справка

- [init](styx-modref-api/init.ru.md), [capture](styx-modref-api/capture.ru.md), [capturelocal](styx-modref-api/capturelocal.ru.md), [adopt](styx-modref-api/adopt.ru.md).
- [Cell.read](styx-modref-api/read.ru.md), [Cell.write](styx-modref-api/write.ru.md), [Cell.payload](styx-modref-api/payload.ru.md), [Cell.close](styx-modref-api/close.ru.md).
