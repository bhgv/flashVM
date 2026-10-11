# In-process channels / Внутренние каналы

**English.** `main.c` is a compiled C89 embedding example using `flashvm/channel.h`. It sends a number and receives it through a FIFO. Channels are independent of a VM context, making them useful as a transport primitive between isolated interpreters once worker-context execution is implemented. The channel owns copied string messages. The receive operation blocks until a message arrives or the channel is closed. `destroy` requires that no other threads are accessing the channel.

**Русский.** `main.c` — компилируемый пример на C89. Число отправляется через FIFO-канал и считывается обратно. Канал не привязан к контексту VM; в будущем это позволит связывать независимые интерпретаторы. Строки копируются. Чтение блокируется до получения сообщения либо закрытия канала. Перед уничтожением нужно дождаться завершения всех потоков, использующих канал.

`channel.as` contains **proposed syntax only**; the AVM1 interpreter does not yet parse channel expressions or operators.

`channel.as` показывает **проектируемый синтаксис**, который пока не исполняется AVM1.
