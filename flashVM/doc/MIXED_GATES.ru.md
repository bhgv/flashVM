# Mixed event scheduling through stream gates


v0.987: Native stream work вынесен под единый callback barrier; base retry, frame gates и EOF-tail сохранены.

v0.988: Добавлен paired pump_mix_until: разрешённый base или event-only работа при сохранении source position и согласованных часов.

v0.989: Проверены TAIL retry перед новым base, удержание BASE, callback-reentry и исключения backend.

v0.990: Mixed worker использует paired gate scheduling: события идут через future gates, retry приоритетен, источники переключаются до event-only подачи.

v0.991: Публикация завершённого кадра отделена от base-разрешения: root Stop держит stream phase, события продолжаются, host pause останавливает очередь.
