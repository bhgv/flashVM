/* Copyright (c) 2026 bhgv
 * EN: Export a bounded, independently owned raster of a MovieClip subtree.
 * RU: Экспорт растра поддерева MovieClip с ограниченным размером и отдельным владением.
 */


/* [GC:] EN: Render clip-local coordinates with a caller-supplied scale and translation.
 * RU: Отрисовать локальные координаты клипа с заданными масштабом и смещением. */
FVM_Result snapshot_bitmap(FVM_Context *c, FVM_Object *clip, unsigned count, const FVM_Value *values,
    FVM_Value *out, void *)
{
    if (!c || !clip || clip->owner != c || clip->resource_kind != CLIP || !out || count != 6 || !values)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    double input[6];
    for (unsigned i = 0; i < 6; ++i)
    {
        FVM_Result result = args(c, count, values, i, 0, input[i]);
        if (result != FVM_OK)
        {
            return result;
        }
    }
    if (input[0] < 1 || input[0] > FVM_BITMAP_MAX_EXTENT || input[0] != std::floor(input[0]) ||
        input[1] < 1 || input[1] > FVM_BITMAP_MAX_EXTENT || input[1] != std::floor(input[1]) ||
        !input[2] || !input[3])
    {
        return FVM_INVALID_ARGUMENT;
    }
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    const unsigned width = (unsigned)input[0], height = (unsigned)input[1];
    const size_t pitch = (size_t)width * 4, bytes = pitch * height;
    const unsigned long memory_limit = c->runtime->config.memory_limit_bytes;
    if (memory_limit && bytes > memory_limit / 2)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_detail::vector<unsigned char> pixels(bytes);
    FVM_RasterSurface surface = {pixels.begin(), width, height, pitch};
    const FVM_Affine projection = {input[2], 0, 0, input[3], input[4], input[5]};
    const double multiply[] = {1, 1, 1, 1}, add[] = {0, 0, 0, 0};
    FVM_Result result = fvm_movieclip_render_bitmap(c, clip, &surface, projection, multiply, add, NULL);
    return result == FVM_OK ? fvm_bitmap_create_pixels(c, width, height, true, pixels, out) : result;
#else
    return FVM_OK;
#endif
}
