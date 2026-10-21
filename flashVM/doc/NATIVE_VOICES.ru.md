# Ограниченные нативные событийные голоса

## v0.783: учёт памяти и предел подготовки

[`fvm_swf_sound_voice_open_bounded`](../include/flashvm/swf_sound.h#L97) сохраняет поведение PCM/ADPCM/MP3, диапазонов,
повторов и огибающих [`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L82). Дополнительный бюджет включает
нативный объект, точный массив выбранного интервала S16 и копии точек огибающей.
Сложения и умножения проверяются до выделения этих владельцев. Даже пустому
голосу нужен объект; нулевой или недостаточный бюджет возвращает OUT_OF_MEMORY
и обнуляет выход. Ограничение полезной нагрузки runtime действует независимо.
Ошибки источника/кодека передаются вызывающему. Стек декодера, память VM,
накладные расходы аллокатора, RSS и очереди устройства не учитываются.
Бюджет ограничивает хранение, а не время вычислений.

[`fvm_swf_sound_voice_storage_info`](../include/flashvm/swf_sound.h#L101) копирует точные размеры объекта/PCM/огибающей
без аллокаций, callbacks и доступа к источнику. Передайте точный struct_size;
ошибка аргументов сохраняет выход. Голос независим от reload, GC и уничтожения
контекста. Доступ требует внешней сериализации; подготовка выполняется владельцем
контекста. Освобождение — voice_close.

Тест `swf_sound_voice_storage`: точная граница и нехватка одного байта для шести
независимо созданных геометрий MPEG и моно/стерео PCM, нулевые указатели,
неверный размер структуры и сохранение результата запроса при ошибке.
Это не подтверждает полноту SWF-звука, эталонное время и работу физических устройств.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_voice_storage_info` | function | [include/flashvm/swf_sound.h:101](../include/flashvm/swf_sound.h#L101) |
<!-- source-links:end -->
