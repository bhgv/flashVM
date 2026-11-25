# rpc.newchan

[English](STYX_RPC_NEWCHAN.md) · [Сессия](STYX_RPC_SESSION.ru.md) · [Реализация](../src/styxrpc/session_channel.c)

Парный плеер FlashVM предоставляет подключённую сессию Styx как `rpc`.
Откройте один обмен UniBus, чтобы передавать значения и живые ссылки через
один fid. При встраивании можно выставить под тем же именем объект,
возвращённый [`fvm_styxrpc_session_bind`](STYX_RPC_SESSION.ru.md).

`rpc.newchan(address)` принимает непустое имя корня обмена другой стороны,
например `"app-events"` или `"/app-events"`. Имя разрешается от корня
подключённой сессии, независимо от текущей папки. Другая сторона публикует
его через [styxrpc_messages_publish](../../lib/styxrpc/reference/messages_publish.ru.md)
с [набором схем канала](../../lib/styxrpc/reference/channel_messages.ru.md).
Укажите один компонент пути длиной до 255 байт с необязательной начальной
косой чертой. Подключённая сессия Styx может идти по локальному или
удалённому транспорту.

Результат владеет отдельным fid с методами [send](STYX_RPC_CHAN_SEND.ru.md),
[recv](STYX_RPC_CHAN_RECV.ru.md) и [close](STYX_RPC_CHAN_CLOSE.ru.md).
Открытие использует срок запроса Styx. Ошибка возникает, если адрес
недопустим или опубликованный обмен открыть не удалось. Объявление типов
другой стороны обрабатывается при чтении.

Этот код AS2 предполагает, что провайдер первым сообщением выдаёт
живую ссылку Int32, а затем принимает значение и ссылку на ту же ячейку.

```actionscript
function onEnterFrame() {
    if (!sent) {
        var ch = rpc.newchan("/app-events");
        var counter = ch.recv();
        var sent = ch.send(counter);
        var forwarded = ch.send(ref counter);
        var closed = ch.close();
    }
}
```

Рабочий контракт эхо-обмена есть в [тестовом провайдере](../tests/styxrpc/variable-ref.c).
Если первый приём не дал полного сообщения, опросите канал на следующем кадре.
Для регистрации собственных схем и обработчиков используйте
[openMessages](STYX_RPC_OPEN_MESSAGES.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_session_bind` | function | [include/flashvm/styxrpc_session.h:36](../include/flashvm/styxrpc_session.h#L36) |
<!-- source-links:end -->


В профиле AS2 View держите операции со ссылками в обработчике кадра.
В коде инициализации временные переменные FIR сейчас разыменовываются
при чтении. Для сквозной проверки соберите [peer-newchan.as](../tests/styxrpc/peer-newchan.as)
через [AS2 View, FIR и SWL](../../compiler/as2/view/README.ru.md) и передайте
путь к SWF первым аргументом тесту `flashvm_tests_styxrpc_variable_ref`.
Тест проверяет, что снимок остался равен 77, а запись через пересланную
ссылку изменила исходное значение на 111.
