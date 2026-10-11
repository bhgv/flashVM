#include "flashvm/font.h"
#include "flashvm/worker.h"
#include "flashvm/channel.h"
#include "fixtures/font_data.h"
#include <assert.h>

static FVM_Result task(FVM_Context *context, const FVM_Value *input, FVM_Value *out, void *userdata)
{
    FVM_Channel *gate = (FVM_Channel *)userdata;
    FVM_Value token;
    FVM_Font *font = 0;
    FVM_TextRun *run = 0;
    FVM_ShapedGlyph glyph;
    unsigned int i;
    (void)context;
    (void)input;
    assert(fvm_channel_receive_owned(gate, &token, 1) == FVM_OK);
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    for (i = 0; i < 100; ++i)
    {
        assert(fvm_font_shape(font, "fi", 2, FVM_TEXT_LTR, "en", &run) == FVM_OK);
        assert(fvm_text_run_count(run) == 1);
        assert(fvm_text_run_get(run, 0, &glyph) == FVM_OK && glyph.glyph == 6);
        fvm_text_run_destroy(run);
    }
    fvm_font_destroy(font);
    fvm_value_number(out, 100);
    return FVM_OK;
}
int main(void)
{
    FVM_Config config;
    FVM_Channel *gate = 0;
    FVM_Worker *workers[8];
    FVM_Value token, out[8];
    unsigned int i;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_channel_create(&gate) == FVM_OK);
    fvm_value_number(&token, 1);
    for (i = 0; i < 8; ++i)
        assert(fvm_worker_run(&config, task, &token, gate, &workers[i]) == FVM_OK);
    for (i = 0; i < 8; ++i)
        assert(fvm_channel_send(gate, &token) == FVM_OK);
    assert(fvm_worker_join_all(workers, 8, out) == FVM_OK);
    for (i = 0; i < 8; ++i)
    {
        assert(out[i].number == 100);
        fvm_worker_destroy(workers[i]);
    }
    fvm_channel_release(gate);
    return 0;
}
