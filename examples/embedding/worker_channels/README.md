# Worker-to-host channel exchange (C89)

`main.c` demonstrates a native worker running in a separate FlashVM context. The host sends a copied UTF-8 string through one FIFO channel, the worker returns its byte length through a second channel, and `join` retrieves the worker's final value. The two channels provide full-duplex communication without sharing Flash objects.

Build with `-DFVM_BUILD_EXAMPLES=ON` and run `flashvm_embedding_worker_channels`. Channel handles are passed through native callback userdata, **not** copied into an AVM1 value. Close channels to wake a blocked receiver; always join workers before destroying their channels.

This example does not implement `thread.run`, `<...>` or `<-` in ActionScript.
