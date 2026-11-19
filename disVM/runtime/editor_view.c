/* Copyright (c) 2026 bhgv
 * EN: Compose owner-local field, input, frame and wait helpers in one C unit.
 * RU: Объединить поле, ввод, кадры и ожидание владельца в одном C-модуле.
 */
/* FlashVM TextField presentation; document/history/file policy lives in Limbo. */
#include "editor_view.h"
#include "flashvm/avm1.h"
#include "flashvm/async.h"
#include "flashvm/timers.h"
#include "ide_view_swf.h"
#include "flashvm/standard_text.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/path.h"
#include "flashvm/render_queue.h"
#include "editor_raster.h"
#include "editor_ink.h"
#include "editor_pointer_gate.h"
#ifdef DVM_EDITOR_SDL
#include "editor_desktop_input.h"
#include "editor_motion_batch.h"
#include "editor_ink_pointer.h"
#include "editor_wheel_batch.h"
#endif
#include "desktop_view.h"
#include "flashvm/utf8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef DVM_EDITOR_SDL
#include <SDL.h>
#include "editor_window_events.h"
#include "editor_text_events.h"
#endif


#include "editor/view/state_contacts.inc"
#include "editor/view/configuration.inc"
#include "editor/view/create_clipboard.inc"
#include "editor/view/selection_geometry.inc"
#include "editor/view/panel_frame.inc"
#include "editor/view/frame_submission.inc"
#include "editor/view/presentation_hits.inc"
#include "editor/view/input_lifecycle.inc"

#include "editor/view/idle_wait.inc"
