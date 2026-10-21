# Явное преобразование частоты и каналов

## Назначение

Настраиваемый микшер объединяет PCM/ADPCM голоса SWF с разными source rate/channels
в один native signed-16 output clock, например 48000 Гц stereo. Работает с
[порционным StartSound](SWF_SOUND_MIXER.ru.md) и [корневым timeline callback](MOVIECLIP_SOUND.ru.md).

Преобразование включается явно. Старый [`fvm_swf_sound_mixer_create`](../include/flashvm/swf_sound_mixer.h#L52) сохраняет
строгую геометрию, прежние ошибки и ABI [`FVM_SWFSoundMixerInfo`](../include/flashvm/swf_sound_mixer.h#L13). Даже в convert
режиме совпадающая source/output геометрия использует прежний прямой путь.
Устройства, потоки, скрытые очереди, compressed decoder и A/V clock не создаются.

## Структура настройки

Отдельная C89 структура [`FVM_SWFSoundMixerConfig`](../include/flashvm/swf_sound_mixer.h#L28); заполнить все поля:

| Поле | Контракт |
| --- | --- |
| struct_size | Точно sizeof([FVM_SWFSoundMixerConfig](../include/flashvm/swf_sound_mixer.h#L28)). |
| sample_rate | Output frames/sec. Strict: 5512/11025/22050/44100. Convert: любое целое от [FVM_SWF_SOUND_MIXER_MIN_RATE](../include/flashvm/swf_sound_mixer.h#L25) (5512) до [FVM_SWF_SOUND_MIXER_MAX_RATE](../include/flashvm/swf_sound_mixer.h#L26) (192000), включая границы. |
| channels | Output mono (1) или interleaved stereo (2). |
| voice_capacity | 1..[FVM_SWF_SOUND_MIXER_MAX_VOICES](../include/flashvm/swf_sound_mixer.h#L9) (64) одновременных голосов. |
| flags | Ноль — strict; [FVM_SWF_SOUND_MIXER_CONVERT](../include/flashvm/swf_sound_mixer.h#L21) — явная линейная конвертация. [`FVM_SWF_SOUND_MIXER_DECLICK`](../include/flashvm/swf_sound_mixer.h#L23) дополнительно включает затухание stop; другие биты запрещены. |

Сохраняется прежняя целая номинальная частота 5512 Гц для SWF 5.5 kHz, не 5512.5.
Формат устройства хост согласовывает отдельно, затем передаёт его rate/channels.

### fvm_swf_sound_mixer_create_configured

`fvm_swf_sound_mixer_create_configured(config, out)` копирует политику и выделяет
микшер, слоты фиксированной ёмкости и рабочие буферы. Out обязателен; *out обнуляется
до остальных проверок. Null config, неверный struct_size, неизвестные flags,
недопустимые rate/channels/capacity дают INVALID_ARGUMENT. Несовместимые размеры
byte/short дают UNSUPPORTED, отказ выделения — OUT_OF_MEMORY. Частичный handle не
публикуется. После OK хост владеет им до close. Config заимствован только на время
вызова; последующие изменения stack struct ничего не меняют. VM, callback и
устройство не нужны. Lookahead добавляет фиксированную память на слот, а не
полную PCM-копию в output rate.

### fvm_swf_sound_mixer_get_config

`fvm_swf_sound_mixer_get_config(mixer, out)` копирует неизменную настройку создания.
Сначала указать точный out->struct_size. Null или неверный размер дают
INVALID_ARGUMENT без изменения out. При OK rate/channels относятся к выходу,
flags показывает политику конвертации. Нет выделений, доступа к VM, callbacks или
продвижения голоса. Это не запрос настоящего hardware format/доступности устройства.
Для старого create возвращается flags=0.

## Алгоритм и округление

Источник читается в позиции `n * source_rate / output_rate`, где n — номер
выходного frame с момента submit. Целый остаток фазы сохраняется между любыми
caller/internal порциями; накопительного floating-point clock нет. Сумма фазы
ограничена output rate плюс SWF source rate и помещается в 32-битный unsigned long.

1. Старый decoder/voice сначала работает на source clock, применяет диапазон,
   повторы и source-time огибающие.
2. Mono→stereo дублирует sample; stereo→mono усредняет left/right с усечением к нулю.
   Например, (-32768 + 32767)/2 даёт 0.
3. Линейная интерполяция между уже mapped/styled source frames использует точную
   целую дробь фазы. Результат каждого голоса усекается к нулю перед сложением в
   широкий аккумулятор. Числители вычисляются через double и представляются точно;
   нестандартный для C++98 long long не требуется. Вся сумма насыщается один раз
   до -32768..32767.

Последний sample удерживается до конца его source-интервала, затем тишина.
N исходных frames с учётом повторов дают `ceil(N * output_rate / source_rate)`
выходных frames. Фаза на границе loop не сбрасывается; интерполяция может пройти
через стык повторов. Отдельное округление длины каждого повтора звук не растягивает.
Это явная host-политика, а не доказательство эквивалентности ресемплеру Flash Player.
**Anti-alias low-pass фильтра нет**: при downsampling возможен aliasing;
качественный band-limited conversion остаётся отдельной работой.

Для mono 44100 Гц [-32768, 32512] выход stereo 48000 Гц:

| Output frame | Left | Right |
| --- | ---: | ---: |
| 0 | -32768 | -32768 |
| 1 | 27208 | 27208 |
| 2 | 32512 | 32512 |
| 3 и далее | 0 | 0 |

## Состояние, ошибки и владение

Submit по-прежнему готовит snapshot до публикации. OOM, codec error и полная
ёмкость сохраняют прежние голоса. Strict отвергает mismatch, convert сохраняет
исходную геометрию и memory budget готового голоса. Второго полного converted PCM
буфера нет. Read не выделяет память и не хранит VM pointers. При завершении голоса
память освобождается; жёсткий real-time allocator/deadline не гарантируется.
Доступ к одному микшеру сериализуется хостом.

Буферизуются текущий и следующий mapped source frame. Decoder может достичь EOF,
пока выход ещё слышим. Такой голос остаётся active и подавляет SyncNoMultiple.
STOP/stop_all немедленно удаляют snapshot и lookahead, но не hardware queue.
Повторно используемый слот начинает с нулевой фазы и без старых frames. Ошибочная
ёмкость и нулевой read не двигают конвертацию. Успешный read всегда заполняет весь
output объём, дополняя конец тишиной. Capacity/written считают выходные **значения
каналов**, не source frames. Reload/GC/уничтожение исходной VM не останавливают
уже подготовленный голос.

## Пример и проверки

[C89 учебник](../examples/multimedia/swf_event_mixer/README.md) получил
`--convert-demo [output-native-s16.pcm]`: настоящий root callback, 48000 Гц stereo,
необязательная реальная запись указанных выше PCM samples через fopen/fwrite/fclose.
Без output path нет file/device I/O. Достаточно NO_MEDIA без SDK. Старые --demo
и --timeline-demo сохраняют strict mono.

Проверяются абсолютный независимый oracle, шесть размеров read, четыре source rate,
одиннадцать output clock, mono/stereo, длинный дробный ratio, 64 голоса, перестановки
voice order, стык loops, отрицательные нечётные средние, buffered EOF и slot reuse.
ADPCM 2..5 бит проходит границу 4096 frames с огибающими/повторами. Есть strict C89,
OOM, teardown VM и реальные interpreter/native JIT counters при подаче timeline
событий в converted mixer. Файл настоящего учебника сравнивается побайтно.
Это software-проверки; приёмка устройств, других ОС и сравнение с оригинальным
проигрывателем остаются открыты.

## Необязательное затухание v0.587

[Политика плеера, новые API и ограничения](PLAYER_AUDIO_RELEASE.ru.md). Прежняя немедленная остановка сохраняется по умолчанию.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../include/flashvm/swf_sound_mixer.h#L21) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:52](../include/flashvm/swf_sound_mixer.h#L52) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:39](../include/flashvm/swf_sound_mixer.h#L39) |
| `FVM_SWF_SOUND_MIXER_DECLICK` | macro / define | [include/flashvm/swf_sound_mixer.h:23](../include/flashvm/swf_sound_mixer.h#L23) |
| `fvm_swf_sound_mixer_get_config` | function | [include/flashvm/swf_sound_mixer.h:49](../include/flashvm/swf_sound_mixer.h#L49) |
| `FVM_SWF_SOUND_MIXER_MAX_RATE` | macro / define | [include/flashvm/swf_sound_mixer.h:26](../include/flashvm/swf_sound_mixer.h#L26) |
| `FVM_SWF_SOUND_MIXER_MAX_VOICES` | macro / define | [include/flashvm/swf_sound_mixer.h:9](../include/flashvm/swf_sound_mixer.h#L9) |
| `FVM_SWF_SOUND_MIXER_MIN_RATE` | macro / define | [include/flashvm/swf_sound_mixer.h:25](../include/flashvm/swf_sound_mixer.h#L25) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:28](../include/flashvm/swf_sound_mixer.h#L28) |
| `FVM_SWFSoundMixerInfo` | struct | [include/flashvm/swf_sound_mixer.h:13](../include/flashvm/swf_sound_mixer.h#L13) |
<!-- source-links:end -->
