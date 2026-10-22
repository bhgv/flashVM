# Mixed event scheduling through stream gates


v0.987: Native stream work вынесен под единый callback barrier; base retry, frame gates и EOF-tail сохранены.

v0.988: Добавлен paired pump_mix_until: разрешённый base или event-only работа при сохранении source position и согласованных часов.

v0.989: Проверены TAIL retry перед новым base, удержание BASE, callback-reentry и исключения backend.
