# Сохранение смешанного stream/event PCM при retry (v0.772)

[English](MIX_QUEUE.md) · [Сложение PCM](STREAM_EVENT_MIX.ru.md) · [C89-пример](../examples/multimedia/swf_stream_mix/README.md)

Нативный адаптер оборачивает один эксклюзивный all-or-none PCM backend. Новый блок
копируется, event-голоса смешиваются один раз. До полного принятия сохраняются
**исходная база и смешанный результат**. Retry обязан совпадать по числу кадров
и всем исходным байтам: посылается сохранённая смесь без второго продвижения голосов.
Полученную callback table можно передать настоящему stream-output producer.

`fvm_swf_sound_mix_queue_create(mixer, backend, block_frames, out, adapted)`
выделяет объект и два фиксированных массива, копирует геометрию и возвращает обычную
[FVM_AudioQueueBackend](../include/flashvm/swf_sound_output.h#L16) table. Не вызывает устройство/pause/clear/VM и не создаёт поток.
Mixer заимствован для эксклюзивного rendering, submit сериализуется хостом. Выходы
ненулевые и не перекрываются; *out обнуляется первым, adapted сохраняется при ошибке.
Backend и adapted могут быть одним descriptor. Нулевой output/mixer, неверные ABI/
callbacks/геометрия или block вне 1..[FVM_SWF_SOUND_OUTPUT_MAX_BLOCK](../include/flashvm/swf_sound_output.h#L11) дают
INVALID_ARGUMENT. Переполнение размера/отказ выделения — OUT_OF_MEMORY с очисткой.

Массивы запрашивают 2 × block_frames × channels × sizeof(short), плюс нативный объект.
Mixer/voices/device и bookkeeping allocator учитываются отдельно. Массивы в callback
не растут. Исходные context/runtime могут быть уничтожены; заимствованные mixer,
userdata устройства и сам адаптер удерживаются до остановки всех пользователей table.

## Callback-контракт

| Операция | Поведение |
| --- | --- |
| write без pending | Копировать базу, один mixer_add, сохранить смесь, вызвать реальную write |
| write с pending | Проверить точное совпадение исходной базы/числа кадров, послать сохранённое |
| write OK | Снять pending после полного копирования реальным backend |
| write error/exception | Сохранить смесь и уже потреблённое время голосов; exception превращается в ERROR |
| queued | Передать реальный count, сохранить output при ошибке/exception |
| pause | Передать только zero/one, не менять pending |
| clear | Снять pending только после реального OK; время голосов не откатывать |

Положительный input — инициализированный выровненный S16 нужной геометрии в пределах
capacity. Нулевой frames без pending — OK без callback/выделения; с pending —
INVALID_ARGUMENT. Изменённый retry отклоняется до backend и без продвижения голосов.
Повторный вход через callback/observation даёт INVALID_ARGUMENT. Статусы backend
передаются, C++ exceptions становятся ERROR с восстановлением busy-guard. Реальный
backend обязан соблюдать прежний атомарный контракт отказа; нарушение откатить нельзя.

`fvm_swf_sound_mix_queue_info(adapter, out)` копирует struct_size, capacity_frames,
pending_frames без устройства/выделений/VM. Null/wrong-size/reentry дают
INVALID_ARGUMENT с неизменным output. [`fvm_swf_sound_mix_queue_close`](../include/flashvm/swf_sound_mix_queue.h#L56) освобождает
только объект/массивы без callback, stop голосов и device close; null допустим.
Вызывать после quiescence всех пользователей table. Нужна внешняя сериализация:
busy-флаг защищает callback recursion, а не гонки потоков. Managed-указателей нет.

## Producer, отмена и хвосты

Создать адаптер до [fvm_swf_stream_output_init](../include/flashvm/swf_stream_output.h#L44), передать producer полученную table.
Producer удерживает исходный stream retry PCM, адаптер — готовую смесь; при отказе
обе ступени удерживают один логический блок. Часы очереди и frame gates остаются
у producer; посторонние write/clear в реальном FIFO нарушают эксклюзивное владение.

Успешный clear отменяет pending/queued смесь, но **не восстанавливает время голосов**.
Согласовать с producer seek/reset и выбранной mixer stop/event политикой. Прямой
вызов clear исходного backend обходит адаптер и нарушает контракт. Close не является
stop. Submit после отказа влияет только на следующие новые блоки, не на pending.

После EOF хост может остановить использование stream producer и явно послать
нулевую базу для event-хвоста. Нельзя затем читать/качать старый producer так, будто
эти добавленные кадры входили в его submitted clock. Desktop всё ещё требует общей
политики timeline/stop/seek/tails; mixed movies пока отклоняются. MP3, дочерние
потоки и A/V synchronization этим checkpoint не добавлены.

## Исполняемые проверки

[C89-пример](../examples/multimedia/swf_stream_mix/README.md) добавляет OUTPUT.pcm
--queue. Настоящий ограниченный native FIFO один раз отклоняет каждый новый блок
до копирования. Адаптер повторяет исходную базу; все 16 stereo кадров совпадают с
прямым экспортом, включая event tail и тишину. Отдельный file consumer запускается
после принятия очередью; частичный fwrite — терминальная ошибка, а не queue retry.
До производства PCM source VM уничтожена.

```sh
cmake --build build --target flashvm_tests_swf_sound_mix_queue \
  flashvm_tests_swf_sound_mix_queue_allocation flashvm_tests_swf_sound_mix_queue_c89 \
  flashvm_examples_multimedia_swf_stream_mix_main
ctest --test-dir build --output-on-failure -R 'swf_sound_mix_queue|swf_stream_mix_tutorial'
./build/flashvm_examples_multimedia_swf_stream_mix_main mixed.pcm --queue
```

Integration fixture использует настоящий stream producer и два event из одного
SWF, write error/throw, неправильный retry, callback recursion, ошибку observation,
clear ack и отдельно разрешённый хвост. Независимый рациональный oracle сравнивает
каждый отсчёт direct/queue режимов. Инъекции выделений проверяют объект/два массива;
после teardown write/retry/info/clear исполняются при запрете выделений. Полная
Flash-совместимость, конкурентность и physical/platform acceptance остаются открыты.

## Границы проверки v0.772

Пройдены 43 нативных sound/stream regression и точный C89 tutorial. ASan/UBSan
покрывают mixer, новый адаптер, producer и новые fixtures; весь core/dependencies
целиком не инструментирован. На ограниченном хосте отключён leak enumeration,
allocation budgets запускаются отдельно; throw callback проверен обычным тестом.

Три новых fixture и tutorial слинкованы для Linux ARM64, RISC-V64, x64 musl и
Windows x64. Проверены 12 ELF64 CPU/static identities и четыре PE32+ набора прямых
OS imports. Реально запускались native Linux x64 и x64 musl; другие CPU, Windows,
настоящие аудиоустройства и полный desktop mixing остаются без приёмки. Новый C API
доступен через статическое ядро; автоматический экспорт каждым shared host не заявлен.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_swf_sound_mix_queue_close` | function | [include/flashvm/swf_sound_mix_queue.h:56](../include/flashvm/swf_sound_mix_queue.h#L56) |
| `fvm_swf_sound_mix_queue_create` | function | [include/flashvm/swf_sound_mix_queue.h:30](../include/flashvm/swf_sound_mix_queue.h#L30) |
| `fvm_swf_sound_mix_queue_info` | function | [include/flashvm/swf_sound_mix_queue.h:52](../include/flashvm/swf_sound_mix_queue.h#L52) |
| `FVM_SWF_SOUND_OUTPUT_MAX_BLOCK` | macro / define | [include/flashvm/swf_sound_output.h:11](../include/flashvm/swf_sound_output.h#L11) |
| `fvm_swf_stream_output_init` | function | [include/flashvm/swf_stream_output.h:44](../include/flashvm/swf_stream_output.h#L44) |
<!-- source-links:end -->
