# Zero-frame sprite definitions / Определения спрайтов без кадров

v0.222 accepts an End-only DefineSprite with a nonzero character ID and a
zero declared frame count. Zero padding after sprite End is accepted; nonzero
trailing data is rejected and the enclosing tag boundary is retained. The dictionary stores total=0 and an empty frame
table; ExportAssets preserves that definition's identity. No synthetic frame
or executable action is inserted. Ordinary nonempty root playback is unchanged.

DefineSprite с ненулевым ID, нулём кадров и End теперь принимается. Нулевое заполнение после End допускается;
ненулевые остаточные данные отвергаются.
В словаре сохраняются total=0 и пустая таблица кадров; ExportAssets ссылается
на тот же объект определения. Искусственный кадр не добавляется.

Primary evidence: unchanged pinned Ruffle SWF6/SWF8 as2_super_and_this_v6/v8
contain zero-frame class symbols with two zero padding bytes after End before
DoInitAction. Ruffle commit
2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/display_object/movie_clip.rs MovieClipShared::with_data retains
header_frames without replacing zero. This is a definition-loading change;
DoInitAction remains unsupported and the original AS2 fixtures still fail.
Instantiation/attachment and playback of zero-frame clips, including empty root
movies, remain open. No full class-loading or roadmap-stage completion is claimed.

Исходный SWF7 function_suppress_and_preload содержит такое определение перед
DoInitAction. Поддержка DoInitAction ещё отсутствует: исходные AS2-тесты не
проходят. Создание экземпляров/attach и исполнение клипов без кадров, включая
пустой корневой фильм, остаются открытыми. Полная загрузка классов не заявляется.

swf_sprite_empty checks SWF5–8 definitions, exports, root frame actions, GC and
transfer to another context without installing its standard library. It verifies
that dictionary/export identity and zero frame count survive transfer and GC.
Malformed streams with ShowFrame despite zero declared frames, missing End or
duplicate character IDs or nonzero trailing padding are rejected. This source-based fixture does not certify
independent complete Flash Player conformance. No ABI or wire-format change.

Тест проверяет метаданные и общую ссылку словаря/export после GC и переноса;
также проверяет ошибочные границы потока и повторные ID. ABI и формат прежние.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 380 and Cairo
Release 379 tests passed after execution-permission retries (6 and 3).
Five selected sprite/timeline/duplicate tests passed in NO_OS_PORTS and
heap/global ASan + UBSan; stack/leak checks remain disabled. All 17 unchanged
selected original SWF<=8 probes retained byte-exact traces and clean exits.
The original AS2 v6/v8 fixtures now pass their empty sprite definitions and stop
with UNSUPPORTED at DoInitAction rather than INVALID_ARGUMENT. The separate
function_suppress_and_preload fixture still fails its declared nonzero sprite
frame-count validation. None of these failing originals is counted as passing.
