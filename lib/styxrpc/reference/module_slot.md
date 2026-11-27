# Module image slot

[Русский](module_slot.ru.md) · [Header](../include/styxrpc/module_slot.h)

A slot serves stable `.upload` and `.state` controls beside one current
loadable image at `mounted/<name>`. Replace the image when the application
selects a new object. The new image gets a separate Qid generation. Open image
fids retire before the previous context can be freed. Controls and the
attached session remain usable. The host can also
[attach a reverse program registry](module_slot_programs.md) beneath the
hidden `.prog` path on the same session.

1. Create the controls and a [slot](module_slot_create.md).
2. Create a regular server with `styxrpc_module_slot_tree` and the slot as context.
3. [Publish the image](module_slot_set.md) on the server owner.
4. Destroy the server, then [release the slot](module_slot_destroy.md),
   the controls, and the last image.

The stock VM pair test calls the image before and after replacing a movie.
[Limbo test](../tests/flashvm/native_player.b)
