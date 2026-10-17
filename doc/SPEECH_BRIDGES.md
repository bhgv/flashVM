# Системная речь / Native speech bridges

## Назначение / Scope

Speech lives in `src/multimedia/speech/tts` (text-to-speech) and `stt`
(speech-to-text), with shared native/JNI ownership helpers in `speech`.
[Public C89 API](../include/flashvm/speech.h) separates explicit output sessions
from explicit recognition/capture sessions. Every new first-party adapter line
is explained; files identify their target OS. Native operations have separate
files. Implementation uses C++98; actual Android services additionally require
Java helpers, and actual macOS STT requires Objective-C++ framework bindings.
No STL container is introduced into first-party C++.

TTS воспроизводит реальный текст через установленный голос/службу. STT принимает
реальную речь через разрешённый микрофон и возвращает распознанный UTF-8 текст.
Здесь нет заранее записанных «распознанных» ответов. При отсутствии SDK, модели,
службы или разрешения нельзя выдавать фиктивный успех. FFmpeg сам по себе не
является TTS/STT движком; speech работает независимо от MEDIA/NO_MEDIA.

| Target | TTS implementation | STT implementation |
| --- | --- | --- |
| Windows | SAPI ISpVoice, real COM voice | SAPI shared recognizer, active dictation grammar, real final events |
| Linux | Explicit filesystem/abstract SSIP Unix socket, real daemon playback | Optional static ALSA capture + Vosk inference, explicit model/device |
| macOS | Speech Synthesis NewSpeechChannel/SpeakCFString | macOS 10.15+ Speech + AVFoundation microphone graph, optional compile switch |
| Android | JNI + supplied FlashVMTts Java helper using TextToSpeech | JNI + supplied FlashVMStt Java helper using SpeechRecognizer |

Функция factory лишь связывает callbacks. Только явный open создаёт нативные
ресурсы, STT open может включать микрофон. Библиотека не вызывает диалоги разрешений,
не устанавливает голоса/модели, не запускает daemon и не скачивает данные.

## Владение, потоки и статусы / Ownership, threading and statuses

Each session copies its exact-size/version backend table, owns one non-null
native stream, and closes it exactly once. Host configuration/userdata, VM,
Context and class references must remain alive through sessions. A failed open
must free all partial native resources itself and leave its output null; common
code then frees only its unpublished wrapper. Tables themselves need no close.

Calls on one session must be serialized, including close. No callback may close
or reenter its own session or throw C++ exceptions across the ABI. Windows calls
stay on the opening STA COM thread, with an ordinary host message pump. macOS
open/close stay on the main thread; the host runs its native main loop. Android
helper work is posted to the app main Looper; native operations may attach a
worker temporarily, detaching only an attachment they created. Java/native
service work is asynchronous, so enqueue acceptance is not audible completion.
Linux SSIP calls are synchronous; each socket I/O wait is bounded by its configured
five-second timeout, **not an overall speech-completion deadline**. Linux Vosk
poll bounds capture frames, not model/inference CPU time.

[GC:] Sessions, native handles, transcript buffers and JNI globals are unmanaged
host resources. They do not point to VM objects and are not charged to its heap.
If a custom provider retains managed values, the host must use real VM roots and
obey VM-thread restrictions. Neither userdata nor a device-thread callback is a
GC root. Closing a context does not close independently owned speech sessions.

[FVM_OK](../include/flashvm/flashvm.h#L42): accepted/completed operation (for TTS, not completion of speaking).
[FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47): asynchronous initialization pending or no transcript yet.
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45): invalid ABI/arguments/Unicode/text limit, or insufficient
STT destination capacity with `required` preserved. [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46): missing
provider/SDK/voice authorization or an unsupported factory target.
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44): identified allocation failure. [FVM_ERROR](../include/flashvm/flashvm.h#L43): native
service/device/inference/protocol failure. SDK errors can share [FVM_ERROR](../include/flashvm/flashvm.h#L43); this
revision does not expose every native error number. Void close cannot return
cleanup errors; call explicit stop first when cancellation status is needed.

## Форматы и callbacks / Data and callback contracts

[FVM_TtsBackend](../include/flashvm/speech.h#L13) contains struct_size, [FVM_SPEECH_ABI_VERSION](../include/flashvm/speech.h#L9), stable os_name,
borrowed userdata and required open/speak/stop/close. Open creates a non-null owned
native state. Speak borrows validated, nonempty NUL-terminated **scalar UTF-8**,
maximum [FVM_SPEECH_TEXT_LIMIT](../include/flashvm/speech.h#L10) payload bytes; common code validates with
[fvm_utf8_validate](../include/flashvm/utf8.h#L24). The provider copies/consumes text before returning. Stop is
session-scoped; close cancels/releases its native state. Native language/voice
selection currently follows system settings; a custom provider can put explicit
voice policy in its userdata. There is no speech-completion or word-boundary API. Windows/Android queue submitted text; macOS SpeakCFString replaces current speech when called while busy, and the SSIP daemon owns its queue/priority policy.

[FVM_SttBackend](../include/flashvm/speech.h#L24) contains the same ABI/configuration fields and required
open/poll/close. Poll returns actual plain UTF-8 and paired final=0/1. Capacity and
required include trailing NUL. Zero capacity queries without consuming. A
successful query must retain **that exact transcript** even if a newer partial
arrives before the next copy. Insufficient capacity returns INVALID_ARGUMENT and
retains text; a sufficient successful copy consumes it. Common code checks
required bounds, finality, complete termination, no embedded NUL and scalar UTF-8
on copying. Failed copy leaves `text[0]=0` when storage exists; inspect `required`
only on successful query or documented insufficient-buffer failure.

Providers may replace older **unqueried** partial hypotheses with the latest;
macOS/Android currently keep one latest queued hypothesis plus one retained query
snapshot. No unbounded transcript queue, confidence/timing/word API or utterance
history is promised. A final text flag does not release capture resources;
always close the session, even after a final transcript or recognition error. Windows currently requests final results only. Close stops
capture; it need not flush an unfinished utterance into a final transcript.

## Подготовка Windows / Windows preparation

1. Install/enable a usable SAPI voice and recognizer language through the OS,
   prepare speakers and the desired system microphone, and permit microphone
   access for the host app under its actual Windows privacy policy.
2. Build with a Windows SDK providing `sapi.h` and native COM libraries. CMake
   links system `sapi`, `ole32`, `uuid`, `user32`. No FFmpeg or model download is
   necessary for SAPI. Installed service/voice availability is a runtime condition.
3. Call factory/open on the intended STA thread. CoInitializeEx success (including
   S_FALSE) is balanced on close. An incompatible existing MTA setup is reported
   as unsupported; use an appropriate host STA thread instead of moving live
   interfaces between threads. Keep the ordinary Windows message pump running.
4. Explicit TTS speak queues plain text (SPF_IS_NOT_XML); stop purges that voice.
   Explicit STT open loads general dictation and activates this client's grammar,
   which can open the configured microphone. Poll reads actual final recognition
   events and copies UTF-16 transcripts to strict UTF-8. Close deactivates only
   this client's grammar and releases COM references.

[Executable TTS preparation](../examples/multimedia/speech_tts/README.md),
[executable STT preparation](../examples/multimedia/speech_stt/README.md).
Native Windows compile/device runs remain required; Linux compilation of guarded
source is not evidence that SAPI was run.

## Подготовка Linux / Linux preparation

TTS: install/configure a real Speech Dispatcher service and an audible output
module according to your distribution. Start it using the distribution's own
service procedure, determine its actual Unix socket, check access as the same
user, and pass that explicit path to [fvm_tts_linux_backend](../include/flashvm/speech.h#L88). There is no assumed
socket, TCP fallback or spawned command. Endpoint strings are configuration,
not shell input. The connection identifies its client, sends SPEAK, normalizes
newlines/dot-stuffs text and checks actual 230/225 protocol replies. CANCEL SELF
never cancels other applications' speech. Disconnect uses MSG_NOSIGNAL and marks
uncertain protocol state failed; close/reopen after failure.

STT: supply actual static ALSA/Vosk SDKs, enable FVM_ENABLE_LINUX_STT, install a
compatible **offline model directory**, prepare the desired permitted ALSA
microphone and pass an explicit device name/model configuration.
[Exact optional SDK layout](../external/speech/README.md) and
[manual STT example](../examples/multimedia/speech_stt/README.md). No model is
chosen/downloaded automatically. Native S16 mono capture is configured at 16 kHz,
at most [FVM_VOSK_CAPTURE_FRAMES](../src/multimedia/speech/stt/linux_vosk.hpp#L15) frames per poll. Empty captures/hypotheses give
NOT_FOUND; overrun/device/inference errors give ERROR, so host decides whether to
close/reopen. Vosk's configured single-field JSON is parsed into plain actual
text, including JSON Unicode escapes and surrogate pairs; no JSON envelope is
exposed as the transcript.

## Именованные Unix-сокеты Android/Linux / Named Unix socket namespaces

Android does support named Unix sockets. Its Java LocalSocketAddress defaults to
Namespace.ABSTRACT; FILESYSTEM and RESERVED are separate namespaces. Native
abstract addresses have `sun_path[0] = '\0'`. The remaining **exact bytes covered
by addrlen** form the name; embedded and trailing NUL bytes are significant.
Abstract addresses are not filesystem paths and are not C strings. A leading
`@` is a display convention in some tools, not the native namespace marker.

[FVM_SpeechUnixEndpoint](../include/flashvm/speech.h#L38) represents namespace plus a byte pointer and explicit
length. For ABSTRACT, caller bytes **exclude the first namespace NUL**, which
the adapter inserts. No extra terminating NUL or structure padding is counted.
For FILESYSTEM, input excludes a terminator, embedded NUL is rejected, and the
adapter appends the pathname terminator. Both require nonempty names fitting the
actual target sockaddr_un capacity. Native length is
`offsetof(sockaddr_un, sun_path) + 1 + endpoint.length`: the extra byte means
namespace marker for abstract, terminator for filesystem.

Use [fvm_tts_unix_backend](../include/flashvm/speech.h#L47) for either namespace on Linux/Android and an actual
installed **SSIP-speaking service**. It does not turn arbitrary LocalSockets or
the Android TTS Binder service into SSIP. Standard Android TextToSpeech remains
the Java/JNI backend. The older [fvm_tts_linux_backend](../include/flashvm/speech.h#L88) is explicitly the Linux
filesystem-path convenience API. The CLI TTS tutorial accepts `--abstract name
text` for printable names; native callers can supply binary names with interior
or trailing NUL using an explicit byte count. Java SocketNames.java demonstrates
real ABSTRACT/FILESYSTEM LocalSocket connections; do not add a NUL to its String.

The runner's EPERM occurs at `socket(AF_UNIX,...)`, before bind/connect or any
name is selected. This is a runner restriction, **not an Android limitation**;
choosing an abstract name does not change that observed point of failure.
Address/length tests need no socket creation. A socketpair transport fixture plus
interposed socket/connect verifies the actual client's binary address and exact
connect length independently; it is not a real abstract endpoint connection.

Primary definitions: [Android LocalSocketAddress](https://developer.android.com/reference/android/net/LocalSocketAddress),
[AOSP namespaces](https://android.googlesource.com/platform/frameworks/base/+/refs/heads/main/core/java/android/net/LocalSocketAddress.java),
[Linux unix(7)](https://www.man7.org/linux/man-pages/man7/unix.7.html).

## Подготовка macOS / macOS preparation

TTS uses actual ApplicationServices Speech Synthesis and the configured system
voice. Its C API is deprecated on newer SDKs but remains an explicit native
adapter; no claim of modern AVSpeech feature parity is made. Keep voice lifecycle
on the host main thread and configure audible output/installed voice beforehand.

STT requires macOS 10.15+, CMake 3.16+ Objective-C++ support, an appropriate SDK and
FVM_ENABLE_MACOS_STT=ON (default on Apple). CMake links Speech, AVFoundation and
Foundation and compiles the `.mm` adapter without ARC, retaining C++98 for its C++
portion. Older SDKs can disable this provider; the STT factory then reports
UNSUPPORTED. The host's deployment target/runtime must actually support these
frameworks; a Linux compile does not verify that deployment.

Host app preparation:

- Provide `NSSpeechRecognitionUsageDescription` and `NSMicrophoneUsageDescription`
  in its real Info.plist. Supply meaningful app-specific explanations.
- Configure its signing/sandbox/hardened-runtime audio-input entitlement
  `com.apple.security.device.audio-input` where the host policy requires it.
- Request real speech and microphone permissions from an explicit user action,
  wait for both outcomes, then open on the main thread. Denied/restricted status
  is respected, never bypassed by the library. Configure a real input device.
- Keep the application main loop running, poll from the serialized host lifecycle,
  and close on the main thread. Capture tap feeds actual AVAudioPCMBuffer objects
  into SFSpeechAudioBufferRecognitionRequest; callbacks retain real Unicode text.
  Close stops/removes the microphone tap, ends input and cancels the native task.

The [macOS preparation source](../examples/multimedia/speech_macos/README.md)
demonstrates actual permission requests and deferred native opening. It is app
integration code, not a standalone cross-platform executable. The system
recognition provider determines availability/network policy; offline processing
is not guaranteed by this adapter.

## Подготовка Android / Android preparation

Use an Android API 23+ host app with a real main Looper, packaged
`org.flashvm.multimedia.FlashVMTts` and `FlashVMStt` helper sources, and a JNI
shared target linking this static FlashVM build. Keep class/package names or
supply the corresponding real class reference through [FVM_AndroidSpeechConfig](../include/flashvm/speech.h#L60).
No Activity is retained: use getApplicationContext(), host-owned JNI **global**
references for Context/helper class, and the actual JavaVM pointer. Resolve app
classes from a Java-originated call with its class loader, not an arbitrary
unattached native worker. Preserve helper names/methods from shrinking/obfuscation.

Manifest/runtime preparation:

- Declare `android.permission.RECORD_AUDIO` and acquire its real runtime grant
  from the Activity before STT open. The helper checks it again and does not
  show a hidden permission dialog. TTS does not need microphone permission.
- For Android 11+ package visibility, declare service-intent queries for
  `android.intent.action.TTS_SERVICE` and `android.speech.RecognitionService`.
- Install/configure a real voice and recognition service/language on the device.
  The STT helper asks EXTRA_PREFER_OFFLINE, which is **advisory**, not a guarantee
  about a provider's network processing. Follow that provider's app/network
  requirements explicitly; no INTERNET permission is silently added by FlashVM.
- Run native open from an explicit user action. Java creates its services on the
  main thread; TTS may return NOT_FOUND until OnInit finishes. Retain requested
  text in the app and retry deliberately. Poll actual STT hypotheses through the
  host UI/event loop; a final result/error ends the utterance, no automatic
  microphone restart occurs. Close in the host lifecycle before releasing JNI
  globals or destroying the VM. Posted shutdown is asynchronous on the Looper.

[Android preparation and JNI example](../examples/multimedia/speech_android/README.md)
includes permission preparation, real global references, UTF-8 byte arrays,
transcript query/copy and cleanup. Standard UTF-8 crosses Java through byte[];
JNI modified-UTF-8 APIs are deliberately not used for transcript content.

Java helper ABI (real source, not mock services):

| Helper | Required methods and behavior | Source |
| --- | --- | --- |
| FlashVMTts | Context constructor; status(); speak(byte[]) → result; stop() → result; close() | [TTS helper](../src/multimedia/speech/tts/FlashVMTts.java) |
| FlashVMStt | Context constructor; status(); take() → marker+UTF-8 byte[] or null; close() | [STT helper](../src/multimedia/speech/stt/FlashVMStt.java) |

TTS queued-service errors are exposed by subsequent status/speak calls; there is
no callback/completion stream yet. STT take() pairs marker and actual hypothesis
atomically; native code keeps an independent copy over size queries. Userdata
and globals are configuration, not transferred ownership. A live VM/Looper
through shutdown is a host prerequisite.

## API reference / Подробный справочник

All functions below obey the shared ownership/thread/result rules above.

### fvm_tts_system_backend

Argument: writable [FVM_TtsBackend](../include/flashvm/speech.h#L13); no prior initialization required. Clears it,
then fills actual current-target callbacks. Null gives INVALID_ARGUMENT.
Windows/macOS give OK when their native adapter is compiled; real voice creation
is deferred. Linux/Android give UNSUPPORTED because an explicit socket/app Context
is needed. No resources are opened and the table itself needs no cleanup.

### fvm_stt_system_backend

Argument: writable [FVM_SttBackend](../include/flashvm/speech.h#L24). Clears it and binds actual Windows SAPI or
compiled macOS Speech callbacks. Null gives INVALID_ARGUMENT; unsupported target
or disabled Apple adapter gives UNSUPPORTED. Factory setup never activates a
microphone or asks permission. Use the explicit Linux/Android factories when
model/device or app configuration is needed.

### fvm_tts_linux_backend

Arguments: nonempty NUL-terminated Unix socket path, writable backend. Clears
backend; invalid path/table or path too long for sockaddr_un gives
INVALID_ARGUMENT. Linux fills a real SSIP table and borrows the path through
native opening; other targets give UNSUPPORTED. The endpoint is copied into the
native socket address during open, never passed to a shell. No connection is made
by this factory. Open can fail with ERROR for socket/daemon/protocol setup.

### fvm_tts_unix_backend

Arguments: exact-size [FVM_SpeechUnixEndpoint](../include/flashvm/speech.h#L38), writable TTS table. Requires an
explicit FILESYSTEM/ABSTRACT namespace, non-null name bytes and nonzero length
fitting native sun_path minus its marker/terminator. Filesystem embedded NUL is
INVALID_ARGUMENT; abstract interior/trailing NUL is preserved exactly. Factory
clears output, validates metadata and borrows config/name through native opening;
Linux/Android return OK, other targets return UNSUPPORTED. Explicit open builds
the exact native length and connects to the real SSIP service, then checks the
actual handshake; failed socket/connection/protocol setup gives ERROR. No daemon
or speech is started by factory setup. Native address rules and Android Java
preparation are detailed above.

### fvm_stt_linux_backend

Arguments: exact-size [FVM_LinuxSttConfig](../include/flashvm/speech.h#L49) with nonempty model_path and alsa_device,
writable backend. Clears table; malformed configuration gives INVALID_ARGUMENT.
Actual Linux static SDK selection gives OK; absent SDK/other OS gives UNSUPPORTED.
Borrow config/strings through sessions. Explicit open loads actual offline model,
creates Vosk inference, opens/configures/starts the named microphone; it cleans
all partial state on failure. Real model/permission/device errors give ERROR.
There is no auto-download, alternate device or fake-recognition fallback.

### fvm_tts_android_backend

Arguments: exact-size [FVM_AndroidSpeechConfig](../include/flashvm/speech.h#L60), writable TTS table. Require actual
non-null JavaVM, global application Context and global helper class references;
invalid configuration gives INVALID_ARGUMENT. Android binds JNI callbacks and
gives OK; other targets give UNSUPPORTED without dereferencing JNI objects.
The factory borrows persistent configuration; no service starts yet. Java ABI,
permission/service and JNI failures during open give ERROR.

### fvm_stt_android_backend

Same configuration/ownership/error rules as [fvm_tts_android_backend](../include/flashvm/speech.h#L69), with the
real FlashVMStt helper class. Explicit open checks already-granted microphone
permission, packages the real recognition service and schedules listening on the
main Looper. Constructor failure is ERROR, never an invented transcript. A final
result or native recognition error does not silently reopen capture.

### fvm_tts_open

Arguments: compatible complete TTS table, writable out_session. Clears output
before validating size/version and all callbacks. Invalid input gives
INVALID_ARGUMENT; common wrapper allocation gives OUT_OF_MEMORY; native callback
failures are preserved; empty successful native handle becomes ERROR. Success
transfers native/common ownership; close once on the prescribed host thread.
Factory does not imply a usable installed voice: actual open can still fail.

### fvm_tts_speak

Arguments: live session, nonempty NUL-terminated scalar UTF-8 string bounded by
[FVM_SPEECH_TEXT_LIMIT](../include/flashvm/speech.h#L10). Missing, oversized or malformed input gives
INVALID_ARGUMENT before invoking any provider. Text is borrowed only during the
call; the actual provider copies it before returning. Returns native result,
including NOT_FOUND during Android initialization. OK means accepted/enqueued
work, not audible completion. Caller must keep session alive until its chosen
stop/close lifecycle; immediate close can cancel just-submitted speech.

### fvm_tts_stop

Argument: live TTS session; null gives INVALID_ARGUMENT. Calls actual native
stop and preserves its result. Cancels only that output's current/queued speech,
keeps its native voice/service usable. Android returns scheduling status;
platform operation is posted to main Looper. Call stop before close if the host
needs a cancellation error result rather than void cleanup.

### fvm_tts_close

Argument: session or null; returns void. Null is safe. Stops/releases native
output then frees common wrapper. Non-null duplicate close or use afterwards is
invalid. Windows balances COM only on the creating thread; Linux closes its real
socket, macOS disposes its native channel, Android schedules real stop/shutdown
and releases owned JNI global references. Host configuration refs remain host-owned.

### fvm_stt_open

Arguments: complete compatible STT table, writable out_session. Clears output,
validates exact ABI and open/poll/close; invalid input gives INVALID_ARGUMENT,
wrapper allocation gives OUT_OF_MEMORY, native errors are preserved. Success
transfers actual recognizer/capture ownership. This is the **explicit microphone
activation boundary**. Obtain permissions and prepare model/service/device first.
Every unsuccessful provider open must release its partial native capture/model.

### fvm_stt_poll

Arguments: live session; char* text (required when capacity>0); size_t capacity;
non-null size_t* required; non-null int* final. Clears scalar outputs and a supplied
nonempty destination first. Invalid shapes give INVALID_ARGUMENT. No-text gives
NOT_FOUND; actual provider errors are preserved. Zero capacity queries and keeps
the pending transcript, reporting required bytes including NUL and final=0/1.
Undersized copy gives INVALID_ARGUMENT with required/final preserved and no text
consumption. Sufficient successful copy consumes the pending event. Common code
rejects malformed successful Unicode/layout/finality as ERROR, empties the
destination and never reads beyond reported capacity. Query/copy must be serialized
with other calls; native callbacks may publish a newer unqueried hypothesis.
No microphone is opened/restarted by polling itself.

### fvm_stt_close

Argument: session or null; void, null-safe. Stops actual native recognition/capture,
releases pending transcripts/model/service refs, then frees the common wrapper.
No final transcript is guaranteed from an unfinished utterance. Close exactly once
and serialize with polling. Windows deactivates this client's grammar, macOS
stops/removes its microphone tap, Linux drops ALSA capture, Android posts actual
cancel/destroy before dropping JNI ownership. Caller still owns its config/globals.

## Свой мост / Writing another real speech provider

Use the same contracts as the native tables. Open the real service/device only
inside open, copy/consume TTS text synchronously, retain real STT output across
query/insufficient-copy attempts, and cancel/release all native resources in close.
Preserve explicit permission and host lifecycle boundaries; declare language,
voice/model, input rate, timestamp/network and threading policy. Avoid fake text,
implicit daemon/model installation or device discovery. Exercise actual SDK error
paths separately from deterministic transcript fixtures. Keep source/OS labels,
every-line comments, source-linked API contracts and prepared-host examples up
to date. [General multimedia/device bridge guide](MULTIMEDIA_BRIDGES.md).

## Проверки и пределы / Evidence and limits

Linux common speech ABI and JSON/Unicode parser run in strict C89/C++98 tests.
Actual SSIP send/recv, multiline replies, dot framing, cancellation and disconnect
are exercised through socketpair protocol fixtures; they **do not verify audible
output**. The separate named Unix-socket endpoint test explicitly skips with code
77 when the runner denies endpoint creation; that is not a daemon integration pass.
ALSA/Vosk adapter units were compiled against actual ALSA 1.2.12/Vosk 0.3.45 headers;
no complete Vosk static/model/microphone run is claimed here. Android JNI units
compiled against the official JNI header; Java adapters and example compiled
against actual Android API 35. These are compile checks, not NDK target binary,
installed-app, microphone or device acceptance. Windows and macOS native runs
remain open. ASAN/UBSAN results apply only to the tests/configuration reported in
the roadmap; no complete roadmap stage is closed.

Primary APIs: [SAPI dictation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms718917(v=vs.85)),
[SSIP](https://github.com/brailcom/speechd/blob/master/doc/ssip.texi),
[Speech Dispatcher architecture](https://freebsoft.org/speechd-in-detail),
[Vosk C API](https://github.com/alphacep/vosk-api/blob/v0.3.45/src/vosk_api.h),
[ALSA PCM](https://www.alsa-project.org/alsa-doc/alsa-lib/group___p_c_m.html),
[Apple Speech](https://developer.apple.com/documentation/speech),
[Android TextToSpeech](https://developer.android.com/reference/android/speech/tts/TextToSpeech),
[Android SpeechRecognizer](https://developer.android.com/reference/android/speech/SpeechRecognizer).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AndroidSpeechConfig` | struct | [include/flashvm/speech.h:60](../include/flashvm/speech.h#L60) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_LinuxSttConfig` | struct | [include/flashvm/speech.h:49](../include/flashvm/speech.h#L49) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SPEECH_ABI_VERSION` | macro / define | [include/flashvm/speech.h:9](../include/flashvm/speech.h#L9) |
| `FVM_SPEECH_TEXT_LIMIT` | macro / define | [include/flashvm/speech.h:10](../include/flashvm/speech.h#L10) |
| `FVM_SpeechUnixEndpoint` | struct | [include/flashvm/speech.h:38](../include/flashvm/speech.h#L38) |
| `fvm_stt_android_backend` | function | [include/flashvm/speech.h:74](../include/flashvm/speech.h#L74) |
| `fvm_stt_close` | function | [include/flashvm/speech.h:106](../include/flashvm/speech.h#L106) |
| `fvm_stt_linux_backend` | function | [include/flashvm/speech.h:57](../include/flashvm/speech.h#L57) |
| `fvm_stt_open` | function | [include/flashvm/speech.h:100](../include/flashvm/speech.h#L100) |
| `fvm_stt_poll` | function | [include/flashvm/speech.h:102](../include/flashvm/speech.h#L102) |
| `fvm_stt_system_backend` | function | [include/flashvm/speech.h:83](../include/flashvm/speech.h#L83) |
| `FVM_SttBackend` | struct | [include/flashvm/speech.h:24](../include/flashvm/speech.h#L24) |
| `fvm_tts_android_backend` | function | [include/flashvm/speech.h:69](../include/flashvm/speech.h#L69) |
| `fvm_tts_close` | function | [include/flashvm/speech.h:98](../include/flashvm/speech.h#L98) |
| `fvm_tts_linux_backend` | function | [include/flashvm/speech.h:88](../include/flashvm/speech.h#L88) |
| `fvm_tts_open` | function | [include/flashvm/speech.h:92](../include/flashvm/speech.h#L92) |
| `fvm_tts_speak` | function | [include/flashvm/speech.h:94](../include/flashvm/speech.h#L94) |
| `fvm_tts_stop` | function | [include/flashvm/speech.h:96](../include/flashvm/speech.h#L96) |
| `fvm_tts_system_backend` | function | [include/flashvm/speech.h:78](../include/flashvm/speech.h#L78) |
| `fvm_tts_unix_backend` | function | [include/flashvm/speech.h:47](../include/flashvm/speech.h#L47) |
| `FVM_TtsBackend` | struct | [include/flashvm/speech.h:13](../include/flashvm/speech.h#L13) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_utf8_validate` | function | [include/flashvm/utf8.h:24](../include/flashvm/utf8.h#L24) |
| `FVM_VOSK_CAPTURE_FRAMES` | macro / define | [src/multimedia/speech/stt/linux_vosk.hpp:15](../src/multimedia/speech/stt/linux_vosk.hpp#L15) |
<!-- source-links:end -->
