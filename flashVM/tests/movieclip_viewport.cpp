/* Host viewport must transform every node once, without changing script-visible geometry/color. */
#define FVM_SHAPE_FIXTURE_ONLY
#include "swf_shape.cpp"
#include <limits>
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_standard_install(context) == FVM_OK);
    Bytes tags, empty;
    tag(tags, 32, shape(9, 32));
    tag(tags, 26, place(6, 1, 9, 40, 0));
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    FVM_Object *root = load(context, tags, 1);
    assert(fvm_movieclip_advance(context, root, 1) == FVM_OK);
    unsigned char original[24 * 24 * 4], translated[24 * 24 * 4];
    FVM_RasterSurface surface = {original, 24, 24, 96};
    FVM_Affine before, after, view = {1, 0, 0, 1, 4, 4};
    assert(fvm_movieclip_get_matrix(context, root, &before) == FVM_OK);
    std::memset(original, 0, sizeof(original));
    std::memset(translated, 0, sizeof(translated));
    FVM_Result rendered = fvm_movieclip_render(context, root, &surface);
    surface.pixels = translated;
    FVM_Result shifted = fvm_movieclip_render_view(context, root, &surface, &view);
    assert(fvm_movieclip_get_matrix(context, root, &after) == FVM_OK);
    assert(before.xx == after.xx && before.xy == after.xy && before.yx == after.yx &&
           before.yy == after.yy && before.x0 == after.x0 && before.y0 == after.y0);
    if (rendered == FVM_OK)
    {
        assert(shifted == FVM_OK);
        for (unsigned y = 0; y < 20; ++y)
            for (unsigned x = 0; x < 20; ++x)
                assert(!std::memcmp(original + 4 * (y * 24 + x),
                                    translated + 4 * ((y + 4) * 24 + x + 4), 4));
        /* Child authored x=2 plus viewport x=4, never two applications of x=4. */
        assert(translated[4 * (8 * 24 + 7)] >= 250 && translated[4 * (2 * 24 + 3) + 3] == 0);
    }
    else
        assert(rendered == FVM_UNSUPPORTED && shifted == FVM_UNSUPPORTED);
    std::memset(translated, 0x55, sizeof(translated));
    view.xx = std::numeric_limits<double>::quiet_NaN();
    assert(fvm_movieclip_render_view(context, root, &surface, &view) == FVM_INVALID_ARGUMENT);
    for (size_t i = 0; i < sizeof(translated); ++i)
        assert(translated[i] == 0x55);
    view.xx = 1;
    assert(fvm_movieclip_render_view(context, root, &surface, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_movieclip_render_view(0, root, &surface, &view) == FVM_INVALID_ARGUMENT);
    FVM_Value invisible;
    fvm_value_boolean(&invisible, 0);
    assert(fvm_object_set(context, root, "_visible", &invisible) == FVM_OK);
    std::memset(translated, 0, sizeof(translated));
    assert(fvm_movieclip_render_view(context, root, &surface, &view) == FVM_OK);
    for (size_t i = 0; i < sizeof(translated); ++i)
        assert(!translated[i]);
    /* Root color remains active through a viewport, just as in ordinary rendering. */
    fvm_value_boolean(&invisible, 1);
    assert(fvm_object_set(context, root, "_visible", &invisible) == FVM_OK);
    fvm_value_number(&invisible, 50);
    assert(fvm_object_set(context, root, "_alpha", &invisible) == FVM_OK);
    std::memset(translated, 0, sizeof(translated));
    shifted = fvm_movieclip_render_view(context, root, &surface, &view);
    if (shifted == FVM_OK)
    {
        unsigned char alpha = translated[4 * (8 * 24 + 7) + 3];
        assert(alpha >= 120 && alpha <= 135);
    }
    else
        assert(shifted == FVM_UNSUPPORTED);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
