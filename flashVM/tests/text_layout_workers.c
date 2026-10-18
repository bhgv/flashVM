#include "flashvm/text_layout.h"
#include "flashvm/worker.h"
#include "flashvm/channel.h"
#include "fixtures/font_data.h"
#include <assert.h>
#ifdef FVM_HAS_FONTS
struct Shared
{
    FVM_Channel *gate;
    FVM_TextLayout *layout;
};
static FVM_Result task(FVM_Context *context, const FVM_Value *input, FVM_Value *out, void *userdata)
{
    struct Shared *shared = (struct Shared *)userdata;
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutOptions options = {57, 0, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    FVM_TextLayoutMetrics metrics;
    FVM_TextLayoutGlyph glyph;
    FVM_Path *copy = 0;
    FVM_Value token;
    unsigned int i;
    (void)context;
    (void)input;
    assert(fvm_channel_receive_owned(shared->gate, &token, 1) == FVM_OK);
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    for (i = 0; i < 50; ++i)
    {
        assert(fvm_text_layout_create(font, "AV fi", 5, &options, &layout) == FVM_OK);
        assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK && metrics.line_count == 2);
        fvm_text_layout_destroy(layout);
        /* Shared layout is read-only and its font has already been destroyed. */
        assert(fvm_text_layout_metrics(shared->layout, &metrics) == FVM_OK &&
               metrics.glyph_count == 3);
        assert(fvm_text_layout_glyph(shared->layout, 0, &glyph) == FVM_OK &&
               glyph.shaped.glyph == 6);
        assert(fvm_text_layout_glyph_path(shared->layout, 0, &copy) == FVM_OK);
        assert(fvm_path_set_point(copy, 0, 0, (double)i, 0) == FVM_OK);
        fvm_path_destroy(copy);
    }
    fvm_font_destroy(font);
    fvm_value_number(out, 50);
    return FVM_OK;
}
#endif
int main(void)
{
#ifdef FVM_HAS_FONTS
    struct Shared shared;
    FVM_Font *font = 0;
    FVM_Config config;
    FVM_Worker *workers[8];
    FVM_Value token, out[8];
    unsigned int i;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_channel_create(&shared.gate) == FVM_OK);
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    assert(fvm_text_layout_create(font, "fiAA", 4, 0, &shared.layout) == FVM_OK);
    fvm_font_destroy(font);
    fvm_value_number(&token, 1);
    for (i = 0; i < 8; ++i)
        assert(fvm_worker_run(&config, task, &token, &shared, &workers[i]) == FVM_OK);
    for (i = 0; i < 8; ++i)
        assert(fvm_channel_send(shared.gate, &token) == FVM_OK);
    assert(fvm_worker_join_all(workers, 8, out) == FVM_OK);
    for (i = 0; i < 8; ++i)
    {
        assert(out[i].number == 50);
        fvm_worker_destroy(workers[i]);
    }
    fvm_text_layout_destroy(shared.layout);
    fvm_channel_release(shared.gate);
#endif
    return 0;
}
