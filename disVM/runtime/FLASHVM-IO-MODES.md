# FlashVM I/O ownership / Владение I/O FlashVM

This records the user's accepted model and the integration boundary after
runtime0.171. **Implemented here:** [purpose-aware code lookup](CODE-LOOKUP.md),
native launch import, typed terminal sessions and existing S-record byte channels.
FlashVM v0.1703 now exposes the [managed player acquisition boundary](../../flashVM/doc/MANAGED_SOURCE.md)
([RU](../../flashVM/doc/MANAGED_SOURCE.ru.md)): purpose-aware startup/root/child SWF and
variable reads, with no native file/HTTP fallback. The [native movie SDK](../native/movie/README.md)
now supplies the actual standard-9P/S-record callback and prepared-VFS service,
validated with real TCP interpreter/JIT nested movies and conflicting providers.
Automatic guest-init/Terminal Session attachment remains pending; this SDK uses
an explicitly granted separate service stream.
**Still to implement:** the remaining FlashVM managed I/O adapter, channel-count lifecycle
messages/policy and per-kind input routing below. No wire schema for these new
commands is assigned by this document, and no FlashVM sandbox switch is changed
before its operations are routed through DisVM.

## Two execution modes / Два режима

| Mode | Code acquisition | Files and network | Input default |
| --- | --- | --- | --- |
| Independent FlashVM | FlashVM's own loader and host resource adapters | FlashVM opens its own granted host/network resources | Local input at the actual FlashVM terminal |
| Started by DisVM | Initial SWF and every nested SWF request go through the DisVM channel, with code intent | Every SWF-facing file/network operation goes through DisVM and its Inferno VFS, using control or granted extra channels | The same local input, until explicitly overridden per kind |

When DisVM starts FlashVM, the start movie is supplied through that connection.
FlashVM still decides *when and which* nested movies to load according to its
normal movie logic; their acquisition goes back through DisVM. Interfaces `.m`,
Dis modules, future native modules and SWF code all use provider-first code
lookup. Ordinary media buffers, configs and other data use ordinary data lookup.
Intent is supplied by the actual operation, not inferred from a user filename.

В управляемом режиме DisVM/VFS — единственный источник прав доступа к файлам
и сети. SWF domain/sandbox-политика доступа не должна независимо разрешать или
запрещать эти операции. Все соответствующие операции сначала переводятся на
канальный VFS-маршрут; отказ DisVM не разрешает прямой доступ к локальным файлам
или сети терминала. В отдельном режиме FlashVM действует самостоятельно.

The adapter must cover initial/nested movie acquisition and every relevant file/
network operation used by supported SWF execution paths. A mode flag alone is
not implementation evidence. Local and remote FlashVM use the same
[Terminal Session](../module/terminal.m) and underlying channel model; endpoint
placement changes routing, not the application's communication interface.

## Additional channels / Дополнительные каналы

The reliable control connection carries the existing
[S-record framing](../../flashVM/doc/SRECORD.md). Direction-local bindings and the
existing [tag-zero size negotiation](../../flashVM/include/flashvm/scontrol.h)
remain in force. Current size negotiation is not a channel-creation API; lifecycle
commands need a separately negotiated service schema before wire implementation.

Accepted behavior for the future channel controller:

1. If queued member count exceeds its configured threshold continuously for the
   configured interval, request **one** additional channel through S tag0.
2. Wait for the correlated admission/ACK before starting another change. The new
   connection belongs to the same authorized job/session and granted resources.
3. Support an ordinary Styx + S-record stream; specialized media profiles may be
   negotiated for audio/video. The FFmpeg integration and payload format remain open.
4. Closing requires both empty queues and low exchange volume for the configured
   interval. Close **one** extra channel at a time, after its pending traffic drains.
5. Keep the primary control connection available for commands and acknowledgements.
   New channel incarnations need distinct association/generation so delayed traffic
   cannot target a replacement stream.

Thresholds, observation times, capacity limits and cooldown are session policy,
not constants fixed by this document. Existing
[size/quality adaptation](../../flashVM/include/flashvm/sadapt.h) is a separate
mechanism; adding/removing channels must not silently resize or reinterpret its
active payload geometry. Each direction/channel maintains its own schema state.
Opening an extra channel does not expand filesystem or device authority.
Queue observation and timing belong to the native channel controller; no Limbo
monitoring loop or guest cooperation is required.

Большие файлы, видео, аудио и иные потоки используют тот же VFS-маршрут через
выданные capability. Создание/закрытие дополнительных каналов идёт по одному
через tag0. Специальный media profile возможен, но формат будущего транспорта
или native-модулей этим решением не предопределяется.

## Selective input control / Выборочное управление вводом

Mouse, touch and keyboard are selected **independently**. Default is local input
at the machine actually running FlashVM. A job may delegate control to its creating
DisVM, whether terminal-local or provider-side. The selected controller is bound
to the authorized session; labels/addresses alone grant no injection authority.

| Per-kind policy | Effective FlashVM input |
| --- | --- |
| Local | Apply local events directly |
| Supplement | Apply local events and additional authorized DisVM events |
| Edit/filter | Send raw local events to DisVM; apply only the returned edited event sequence, which may omit, change or add events |
| Replace | Suppress local events for this kind; apply the authorized DisVM event stream, including nonstandard input-device sources |

Для редактируемого вида ввода исходное событие не применяется до обработки:
DisVM получает его, меняет/фильтрует/дополняет и возвращает эффективный ввод.
При полной замене используется поток DisVM. Не выбранные виды ввода продолжают
обычную локальную обработку FlashVM.

The protocol will need event correlation and policy generations: returned edited
events must not be recirculated as new raw input, and replies for a retired policy
must not create duplicate/stale events. Changing policy must reconcile pressed
keys/buttons and active touch contacts. Queue and disconnect behavior must be
explicit; an unresolved editing request cannot silently switch to unfiltered input.
These are integration requirements, not a finalized event wire layout.

Input routing remains event/channel based. It does not require suspending a guest,
global VM stop or cooperative polling. Resource permission decisions stay with
DisVM/VFS; FlashVM owns application of the resulting authorized input/rendering.
