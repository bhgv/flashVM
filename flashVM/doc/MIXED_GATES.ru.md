# Mixed event scheduling through stream gates


v0.987: Native stream work вынесен под единый callback barrier; base retry, frame gates и EOF-tail сохранены.

v0.988: Добавлен paired pump_mix_until: разрешённый base или event-only работа при сохранении source position и согласованных часов.

v0.989: Проверены TAIL retry перед новым base, удержание BASE, callback-reentry и исключения backend.

v0.990: Mixed worker использует paired gate scheduling: события идут через future gates, retry приоритетен, источники переключаются до event-only подачи.

v0.991: Публикация завершённого кадра отделена от base-разрешения: root Stop держит stream phase, события продолжаются, host pause останавливает очередь.

v0.992: Проверены seeded границы 2^53, backend/clock rejection и отсутствие выделений в gate/retry/clear после подготовки.

v0.993: Выполнены 256 native-эпох на четырёх OS-потоках: VM teardown, failed tail retry, смена gates, точный PCM и dequeue/ID.

v0.994: Удержанный BASE сохранён; проверены drain принятого lead, failed clear и новые события остановленного корня после staged cancellation.

v0.995: Добавлен седьмой gated-режим C89: 17 точных кадров, неизменный converter при удержании и failed TAIL-to-BASE переходы.
