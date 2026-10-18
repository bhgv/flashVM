/* FlashVM: rooted values used while preparing a semantic timeline request. */
#ifndef FVM_FRAME_REQUEST_CONSTANTS_H
#define FVM_FRAME_REQUEST_CONSTANTS_H   /* Timeline-request preparation guard. */
#define FVM_FRAME_REQUEST_INPUT_ROOT 0U /* [GC:] Original value during string conversion. */
#define FVM_FRAME_REQUEST_TEXT_ROOT 1U  /* [GC:] Converted frame/label retained until publication. \
                                         */
#define FVM_FRAME_REQUEST_ROOT_COUNT 2U /* [GC:] Original and converted request values. */
#endif
