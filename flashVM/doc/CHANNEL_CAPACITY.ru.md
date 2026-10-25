# Ограниченная ёмкость каналов (C API)

`fvm_channel_set_capacity(channel, count)` задаёт максимальное количество сообщений в очереди. Ноль (значение по умолчанию) означает отсутствие ограничения. При заполнении очереди `fvm_channel_send()` блокируется до получения сообщения потребителем либо закрытия канала. `fvm_channel_pending()` возвращает синхронизированный снимок длины очереди. `fvm_channel_close()` пробуждает ожидающих отправителей и получателей; заблокированная отправка после закрытия возвращает [`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Уже накопленные сообщения можно прочитать.

Функции доступны из C89 и C++98. Нельзя уничтожать канал, пока им пользуется другой поток. Для предсказуемого ограничения нагрузки задавайте ёмкость до запуска отправителей. Операторы каналов ActionScript этим изменением не реализуются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_close` | function | [include/flashvm/channel.h:85](../include/flashvm/channel.h#L85) |
| `fvm_channel_pending` | function | [include/flashvm/channel.h:151](../include/flashvm/channel.h#L151) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_set_capacity` | function | [include/flashvm/channel.h:146](../include/flashvm/channel.h#L146) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
