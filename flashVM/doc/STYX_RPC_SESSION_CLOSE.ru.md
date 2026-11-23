# fvm_styxrpc_session_close

[English](STYX_RPC_SESSION_CLOSE.md) · [Заголовок](../include/flashvm/styxrpc_session.h) · [Исходник](../src/styxrpc/session.c) · [Привязка](STYX_RPC_SESSION.ru.md)

`void fvm_styxrpc_session_close(FVM_StyxRpcSession *binding);`

Вызовите в потоке владельца FlashVM между вызовами методов. Функция
отключает объект AS, закрывает все принадлежащие ему fid и отзывает токены.
Clunk запрашивает отмену ещё работающих программ. Клиент Styx и встречный
реестр остаются во владении хоста, который может закрыть их после возврата.
Храните привязку до последней вызываемой ссылки на связанный объект AS.
NULL и повторное закрытие допустимы. Повторно зарегистрировать привязку
нельзя. Вызов может ожидать завершения операций clunk сессии.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_styxrpc_session_close` | function | [include/flashvm/styxrpc_session.h:42](../include/flashvm/styxrpc_session.h#L42) |
| `FVM_StyxRpcSession` | struct | [include/flashvm/styxrpc_session.h:24](../include/flashvm/styxrpc_session.h#L24) |
<!-- source-links:end -->
