# Привязка подготовленного вывода к StopSounds — v0.583

[English](AUDIO_STOP_BINDINGS.md).
[`FVM_AudioStopBinding`](../src/multimedia/audio/channel.hpp#L13) связывает native-микшер/очередь host с AVM1 StopSounds,
стандартным stopAllSounds и [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L92) контекста. Подготовленный голос
может существовать до первой записи PCM; смысл active у прежних PCM channels
не меняется. Фиктивная отправка samples не нужна.
[Исполняемый C89 пример](../examples/multimedia/stop_binding/README.md).
[Настоящий SWF плеера](../examples/player/animation_audio/03-stop-sounds.swf).

## Callback и порядок

[`FVM_AudioStopCallback`](../include/flashvm/audio_stop.h#L10) принимает заимствованный native userdata, возвращает
[`FVM_Result`](../include/flashvm/flashvm.h#L40) и идемпотентно останавливает принадлежащие ему голоса/pending PCM/
очереди. Binding не открывает устройство и не навязывает формат/ОС/микшер.
SDL-плеер вызывает [`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L90): pause/clear/reset своего вывода.

[`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L92) сначала пытается остановить активные PCM channels в порядке
регистрации, затем **все** bindings в их порядке регистрации, даже без принятого
PCM. Неактивные channels по-прежнему пропускаются. Bindings вызываются всегда:
VM не знает состояние подготовки внешнего микшера. Успех не удаляет binding;
следующий stop и уничтожение контекста снова вызывают callback. Уже остановленное
состояние обязательно допустимо.

После ошибки выполняются остальные callback. Возвращается первый non-OK результат
в этом порядке. C++ bad_alloc преобразуется в [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44), остальные
исключения — [`FVM_ERROR`](../include/flashvm/flashvm.h#L43), отдельно для каждого callback. Та же граница теперь
защищает channel stop/close и terminal cleanup channels. Ошибка не удаляет binding;
следующий явный stop повторяет вызов. Автоматического retry/отката ранее
успешных независимых остановок нет.

## Создание

`fvm_audio_stop_binding_create(context, callback, userdata, out)` копирует callback/
userdata в native-регистрацию, публикует принадлежащий контексту opaque handle и
даёт [`FVM_OK`](../include/flashvm/flashvm.h#L42). Callback не вызывается, managed values не сохраняются.
out обязателен; *out очищается при любой ошибке. Требуется живой context своего
потока вне GC/teardown/audio callback, ненулевой callback. Нарушение параметров/
фазы: [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Null userdata допустим, если callback его принимает.

Лимит [`FVM_AUDIO_STOP_MAX_BINDINGS`](../include/flashvm/audio_stop.h#L8) (64) независим от PCM channels. Исчерпание
лимита/памяти даёт [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); прочие native exceptions — [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).
Ошибка allocation/insertion не оставляет entry и не меняет существующие binding.
Владение при ошибке не передаётся. Успешная регистрация относится к context,
не runtime, movie, значению global function или клонированному контексту.

## Закрытие

`fvm_audio_stop_binding_close(binding)` удаляет регистрацию/обёртку, даёт [`FVM_OK`](../include/flashvm/flashvm.h#L42).
Null допустим. **Stop не вызывается**, поэтому detach доступен после необратимой
ошибки устройства. Если нужен результат остановки, сначала явный stop; затем
detach до освобождения заимствованного mixer/device/userdata. Живой binding в
запрещённой фазе context/callback даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), сохраняя регистрацию.
Выделения памяти/callback нет. Успешный close инвалидирует handle. Произвольный/
висячий указатель не является валидируемым handle: после close/context destruction
использовать его нельзя.

## Владение, GC, потоки и перенос

Код callback/userdata принадлежат host и живут до detach либо окончания context
destruction. GC не сканирует raw userdata: managed objects закрепляются отдельно,
предпочтительнее полностью native control. Только owning thread, как у прочих
context API. Audio callback guard запрещает изменение registry, nested stop-all,
GC; защищённый void context_destroy игнорируется. Это не mutex, отмена worker или
API для signal handler. Callback не должен бесконечно блокироваться/уничтожать VM.

При уничтожении context все bindings вызываются один раз после active channels;
исключения преобразуются, terminal errors игнорируются, регистрации освобождаются,
handles инвалидируются. Host resources должны жить до возврата destruction.
Для проверки stop status требуется явная остановка заранее. Detach до освобождения
userdata предотвращает terminal callback к уже уничтоженному устройству.

Регистрации process-local, не входят в graph serialization. Перенесённый стандартный
stopAllSounds сохраняет стандартный callback identity и действует на собственные
controls целевого контекста, не копируя userdata исходного. Прежние ограничения
переноса native functions не меняются.

## SDL-плеер

[frontend/common/audio.c](../frontend/common/audio.c) регистрирует control после
mixer/output, до timeline callback. StartSound→StopSounds в одном кадре удаляет
подготовленный голос даже до первого приёма PCM. Удаляются также pending после
ошибки write и принятая очередь. Ошибка clear оставляет paused/pending для явного
retry, binding сохраняется. Native worker может заполнить тишину и resume;
последующие StartSound снова звучат. Cleanup сначала detach, затем stop/close
producer/mixer/device до window SDL_Quit. Context channels и stop по ID не меняются.
--mute не создаёт ни output, ни binding; --stats добавляет успешные sound_stops.

[03-stop-sounds.swf](../examples/player/animation_audio/03-stop-sounds.swf) отменяет
звук frame0, обрывает loop frame30 и позднее запускает stereo PCM/ADPCM.
Это настоящие AVM1 opcodes в interpreter/JIT, не имитация trigger из host.
Оба исходных SWF v0.582 побайтно сохранены. AS Sound, sprite sound, точное время
ЦАП и drift-corrected A/V sync остаются открытыми.

## Границы проверки

Регрессионные тесты: C89 ABI, OOM/cap rollback, stop/detach без новых allocations,
изоляция contexts, GC/reentry/destruction guards, исключения, first-error/attempt-all
и настоящий interpreter/JIT с удалением pending PCM. Отдельный frontend-прогон
использует реальный SDL SDK и dummy audio под X11/Xvfb. Это протокол, не слышимость
и не нативная приёмка Windows/Android/macOS. [Статус](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:92](../include/flashvm/multimedia.h#L92) |
| `fvm_audio_stop_binding_close` | function | [include/flashvm/audio_stop.h:17](../include/flashvm/audio_stop.h#L17) |
| `fvm_audio_stop_binding_create` | function | [include/flashvm/audio_stop.h:14](../include/flashvm/audio_stop.h#L14) |
| `FVM_AUDIO_STOP_MAX_BINDINGS` | macro / define | [include/flashvm/audio_stop.h:8](../include/flashvm/audio_stop.h#L8) |
| `FVM_AudioStopBinding` | struct | [src/multimedia/audio/channel.hpp:13](../src/multimedia/audio/channel.hpp#L13) |
| `FVM_AudioStopCallback` | callback typedef | [include/flashvm/audio_stop.h:10](../include/flashvm/audio_stop.h#L10) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:90](../include/flashvm/swf_sound_output.h#L90) |
<!-- source-links:end -->
