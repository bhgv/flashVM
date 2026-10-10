# INPUT_API — русская документация

Приложение-хост распознаёт касания и свайпы, а FlashVM сопоставляет их с действиями мыши. Для синхронного получения событий используйте `fvm_input_set_callback` и `fvm_input_dispatch`. Подробности: `INPUT_EVENTS.md`. Автоматической передачи в `MovieClip.onPress` пока нет.

Подробные сигнатуры и ограничения см. в `INPUT_API.md` и заголовках `include/flashvm/`. Примеры расположены в `examples/embedding/` и `examples/extending/`.
