/* Copyright (c) 2026 bhgv
 * EN: Compose desktop scene decoding, painting, motion and lifetime helpers.
 * RU: Объединить разбор desktop-сцены, рисование, движение и управление жизнью.
 */
/* Actual MovieClip/AS widget presentation for copied Limbo desktop scenes. */
#include "desktop_view.h"
#include "vector_local.h"
#include "desktop_chrome.h"
#include "editor_ink.h"
#include "editor_vector_layout.h"
#include "desktop_view_swf.h"
#include "flashvm/avm1.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/standard_text.h"
#include "flashvm/text_layout.h"
#include "flashvm/gc.h"
#include "flashvm/utf8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include "editor/desktop/state_motion.inc"
#include "editor/desktop/bindings_vectors.inc"
#include "editor/desktop/vector_paint.inc"
#include "editor/desktop/scene_decode.inc"
#include "editor/desktop/text_selection.inc"
#include "editor/desktop/syntax_gutter.inc"
#include "editor/desktop/widget_paint.inc"
#include "editor/desktop/retirement_cache.inc"
#include "editor/desktop/frame_lifecycle.inc"


/* EN: Query active visual interpolation on the FlashVM owner. No parameters,
 * clock sampling, callbacks or mutation; zero means no animation wake is needed.
 * RU: Проверить анимацию на владельце FlashVM. Нет параметров, чтения часов,
 * callbacks или мутации; ноль означает отсутствие нужды в animation wake. */
int dvm_desktop_animating(void)
{
    unsigned int id;
    if (!active || pending)
    {
        return 0;
    }
    if (camera_motion.active)
    {
        return 1;
    }
    for (id = 1; id <= 8; id++)
    {
        if (motions[id].active)
        {
            return 1;
        }
    }
    return 0;
}
