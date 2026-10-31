/* Select independently compiled AS views; geometry/colors live in their sources. */
#include "desktop_chrome.h"
#include "flashvm/avm1.h"
#include "chrome_desktop_background.h"
#include "chrome_window_card.h"
#include "chrome_miniature_card.h"
#include "chrome_window_header.h"
#include "chrome_tool_scroll.h"
#include "chrome_tab_button.h"
#include "chrome_input_field.h"
#include "chrome_top_bar.h"
#include "chrome_rail_left.h"
#include "chrome_rail_right.h"


/* Publish copied scalars to an owner-local AS view. No managed value crosses Dis. */
static int parameter(FVM_Context *owner, const char *name, double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_set_global(owner, name, &value) == FVM_OK;
}


/* Run already linked AS actions on this authority thread. The generic property
 * adapter supplies drawing primitives; AS owns every colour and shape parameter.
 * This uses the supported executable AS/FIR profile, not the full AS language. */
static int execute(FVM_Context *owner,
                   FVM_Object *clip,
                   const unsigned char *actions,
                   size_t size,
                   double wide,
                   double high,
                   int active,
                   int selected)
{
    return dvm_chrome_drawing_bind(owner, clip) && parameter(owner, "chromeWidth", wide) &&
           parameter(owner, "chromeHeight", high) && parameter(owner, "chromeActive", active) &&
           parameter(owner, "chromeSelected", selected) && fvm_avm1_execute(owner, actions, size) == FVM_OK;
}


/* Read one ordinary managed cache scalar. A fresh clip has no cache values.
 * No native pointer is retained; re-created objects cannot inherit a stale key. */
static int cached_number(FVM_Context *owner, FVM_Object *clip, const char *name, double expected)
{
    FVM_Value value;
    return fvm_object_get(owner, clip, name, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
           value.number == expected;
}


/* Store one scalar cache field on its owning MovieClip; no cross-heap state. */
static int cache_number(FVM_Context *owner, FVM_Object *clip, const char *name, double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_object_set(owner, clip, name, &value) == FVM_OK;
}


/* Identify AS presentation roles from copied scene identities, without styling them. */
int dvm_chrome_owns_view(unsigned long id, unsigned long mirror)
{
    return id <= 8 || id == 10 || (id >= 13 && id <= 19) || id == 28 || (id >= 30 && id <= 37) ||
           (id >= 41 && id <= 59) || id == 63 || (id >= 64 && id <= 85) || mirror;
}


/* Route copied view roles to AS resources, never calculate their visual geometry. */
static int render_view(FVM_Context *owner, FVM_Object *clip, const DVM_ChromeItem *item, double wide, double high)
{
    if (item->id == 10)
    {
        return execute(owner, clip, chrome_desktop_background, sizeof(chrome_desktop_background), wide, high, 0, 0);
    }
    if (item->id <= 8 || (item->id >= 13 && item->id <= 19) || item->id == 28 || item->id == 55 || item->mirror)
    {
        const unsigned char *card = item->mirror ? chrome_miniature_card : chrome_window_card;
        size_t size = item->mirror ? sizeof(chrome_miniature_card) : sizeof(chrome_window_card);
        return parameter(owner, "chromePreviewScale", item->preview_scale) &&
               parameter(owner, "chromeMiniature", item->mirror != 0) &&
               parameter(owner, "chromePreviewWidth", item->preview_width) &&
               parameter(owner, "chromePreviewHeight", item->preview_height) &&
               execute(owner, clip, card, size, wide, high, item->active, 0) &&
               execute(owner, clip, chrome_window_header, sizeof(chrome_window_header), wide, high, item->active, 0);
    }
    if (item->id == 57)
    {
        return execute(owner, clip, chrome_input_field, sizeof(chrome_input_field), wide, high, 0, 0);
    }
    if (item->id == 63)
    {
        return execute(owner, clip, chrome_top_bar, sizeof(chrome_top_bar), wide, high, 0, 0);
    }
    if (item->id >= 30 && item->id <= 37)
    {
        return execute(owner, clip, chrome_tab_button, sizeof(chrome_tab_button), wide, high, item->active, 0);
    }
    if ((item->id >= 41 && item->id <= 59) || (item->id >= 64 && item->id <= 85))
    {
        return execute(owner, clip, chrome_tool_scroll, sizeof(chrome_tool_scroll), wide, high, 0, item->selected);
    }
    return 1;
}


/* Reuse unchanged owner graphics. Source text/caret jobs stay independent.
 * Vector panes explicitly repaint their clean body before appending the latest
 * path snapshot. Cache metadata is managed with the clip and dies with it. */
int dvm_chrome_decorate(FVM_Context *owner, FVM_Object *clip, const DVM_ChromeItem *item, double wide, double high)
{
    if (!item->dynamic && cached_number(owner, clip, "_chromeWidth", wide) &&
        cached_number(owner, clip, "_chromeHeight", high) &&
        cached_number(owner, clip, "_chromeActive", item->active) &&
        cached_number(owner, clip, "_chromeSelected", item->selected) &&
        cached_number(owner, clip, "_chromePreviewWidth", item->preview_width) &&
        cached_number(owner, clip, "_chromePreviewHeight", item->preview_height))
    {
        return 1;
    }
    return render_view(owner, clip, item, wide, high) && cache_number(owner, clip, "_chromeWidth", wide) &&
           cache_number(owner, clip, "_chromeHeight", high) &&
           cache_number(owner, clip, "_chromeActive", item->active) &&
           cache_number(owner, clip, "_chromeSelected", item->selected) &&
           cache_number(owner, clip, "_chromePreviewWidth", item->preview_width) &&
           cache_number(owner, clip, "_chromePreviewHeight", item->preview_height);
}


/* Cache only immutable rail geometry. Extents belong to the caller's context;
 * reset them at context teardown. Colours/fade distribution are AS source policy. */
int dvm_chrome_glass(FVM_Context *owner,
                     FVM_Object *clip,
                     double wide,
                     double high,
                     int right,
                     unsigned int *cached_width,
                     unsigned int *cached_height)
{
    int ok;
    if (*cached_width == (unsigned int)wide && *cached_height == (unsigned int)high)
    {
        return 1;
    }
    ok = right ? execute(owner, clip, chrome_rail_right, sizeof(chrome_rail_right), wide, high, 0, 0)
               : execute(owner, clip, chrome_rail_left, sizeof(chrome_rail_left), wide, high, 0, 0);
    if (ok)
    {
        *cached_width = (unsigned int)wide;
        *cached_height = (unsigned int)high;
    }
    return ok;
}
