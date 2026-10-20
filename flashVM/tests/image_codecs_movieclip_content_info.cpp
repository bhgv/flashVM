/* Image content records describe original encoded/pixel payloads, not transformed drawing bounds.
 */
#define FVM_IMAGE_CLIP_FIXTURE_ONLY
#include "image_codecs_movieclip_loading.cpp"
#include "flashvm/serialization.h"
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
    FVM_Object *clip = make_child(c, root, "image");
    Bytes images[] = {png_fixture(), gif_fixture(true), jpeg_fixture()};
    FVM_MovieClipContentFormat formats[] = {FVM_MOVIECLIP_CONTENT_PNG, FVM_MOVIECLIP_CONTENT_GIF,
                                            FVM_MOVIECLIP_CONTENT_JPEG};
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        assert(fvm_movieclip_replace_image(c, clip, images[kind].begin(), images[kind].size(),
                                           "image.bin", 9) == FVM_OK);
        FVM_MovieClipContentInfo info;
        assert(fvm_movieclip_content_info_init(&info) == FVM_OK);
        assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK);
        assert(info.format == formats[kind] && info.encoded_bytes == images[kind].size() &&
               !info.swf_version);
        assert(info.pixel_width == 8 && info.pixel_height == 8 && info.decoded_bytes == 256);
        FVM_Value count;
        assert(fvm_object_call(c, clip, "getBytesLoaded", 0, 0, &count) == FVM_OK &&
               count.number == images[kind].size());
        assert(fvm_object_call(c, clip, "getBytesTotal", 0, 0, &count) == FVM_OK &&
               count.number == images[kind].size());
        assert(fvm_object_call(c, clip, "getSWFVersion", 0, 0, &count) == FVM_OK &&
               count.number == -1);
        FVM_MovieClipContentInfo before = info;
        FVM_Value value;
        fvm_value_number(&value, 16);
        assert(fvm_object_set(c, clip, "_width", &value) == FVM_OK);
        assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK &&
               !std::memcmp(&info, &before, sizeof(info)));
        assert(fvm_movieclip_replace_image(c, clip, images[kind].begin(), images[kind].size() / 2,
                                           "bad", 3) != FVM_OK);
        assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK &&
               !std::memcmp(&info, &before, sizeof(info)));
        /* Duplicate content keeps immutable metadata, but unloading the duplicate cannot reset it.
         */
        FVM_Value args[2], out;
        fvm_value_string(args, "copy");
        fvm_value_number(args + 1, 2);
        assert(fvm_object_call(c, clip, "duplicateMovieClip", 2, args, &out) == FVM_OK);
        FVM_Object *copy = out.object;
        assert(fvm_movieclip_get_content_info(c, copy, &info) == FVM_OK &&
               info.encoded_bytes == images[kind].size());
        assert(fvm_movieclip_unload(c, copy) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_movieclip_get_content_info(c, clip, &info) == FVM_OK &&
               info.format == formats[kind]);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
