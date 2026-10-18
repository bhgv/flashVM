#include "gif_lib.h"
#include "image_codecs_memory.h"
#include <assert.h>

static int write_bytes(GifFileType *gif, const GifByteType *bytes, int count)
{
    TestImageBytes *out = (TestImageBytes *)gif->UserData;
    if (count < 0 || (size_t)count > sizeof(out->data) - out->size)
        return 0;
    memcpy(out->data + out->size, bytes, (size_t)count);
    out->size += (size_t)count;
    return count;
}
static int read_bytes(GifFileType *gif, GifByteType *bytes, int count)
{
    TestImageBytes *input = (TestImageBytes *)gif->UserData;
    size_t available = input->size - input->position;
    if (count < 0)
        return 0;
    if ((size_t)count > available)
        count = (int)available;
    memcpy(bytes, input->data + input->position, (size_t)count);
    input->position += (size_t)count;
    return count;
}
int main(void)
{
    TestImageBytes bytes;
    GifFileType *gif;
    ColorMapObject *palette;
    GifColorType colors[2];
    GifPixelType row[8];
    GraphicsControlBlock control, read_control;
    GifByteType extension[4];
    int error = 0, x, y;
    memset(&bytes, 0, sizeof(bytes));
    memset(colors, 0, sizeof(colors));
    colors[1].Red = 255;
    palette = GifMakeMapObject(2, colors);
    assert(palette);
    gif = EGifOpen(&bytes, write_bytes, &error);
    assert(gif);
    EGifSetGifVersion(gif, 1);
    assert(EGifPutScreenDesc(gif, 8, 8, 8, 0, palette) == GIF_OK);
    control.DisposalMode = DISPOSE_BACKGROUND;
    control.UserInputFlag = 0;
    control.DelayTime = 12;
    control.TransparentColor = 0;
    assert(EGifGCBToExtension(&control, extension) == 4);
    assert(EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, 4, extension) == GIF_OK);
    assert(EGifPutImageDesc(gif, 0, 0, 8, 8, 0, 0) == GIF_OK);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
            row[x] = (GifPixelType)((x + y) % 2);
        assert(EGifPutLine(gif, row, 8) == GIF_OK);
    }
    /* A second frame tests animation and a local color map. */
    assert(EGifPutImageDesc(gif, 2, 3, 2, 2, 0, palette) == GIF_OK);
    row[0] = row[1] = 1;
    assert(EGifPutLine(gif, row, 2) == GIF_OK);
    assert(EGifPutLine(gif, row, 2) == GIF_OK);
    assert(EGifCloseFile(gif, &error) == GIF_OK);
    GifFreeMapObject(palette);
    assert(bytes.size > 20 && memcmp(bytes.data, "GIF89a", 6) == 0);
    gif = DGifOpen(&bytes, read_bytes, &error);
    assert(gif && DGifSlurp(gif) == GIF_OK);
    assert(gif->SWidth == 8 && gif->SHeight == 8 && gif->ImageCount == 2);
    for (y = 0; y < 8; ++y)
        for (x = 0; x < 8; ++x)
            assert(gif->SavedImages[0].RasterBits[y * 8 + x] == (x + y) % 2);
    assert(DGifSavedExtensionToGCB(gif, 0, &read_control) == GIF_OK);
    assert(read_control.TransparentColor == 0 && read_control.DelayTime == 12);
    assert(read_control.DisposalMode == DISPOSE_BACKGROUND);
    assert(gif->SavedImages[1].ImageDesc.ColorMap->Colors[1].Red == 255);
    assert(gif->SavedImages[1].ImageDesc.Left == 2 && gif->SavedImages[1].ImageDesc.Top == 3);
    assert(DGifCloseFile(gif, &error) == GIF_OK);
    bytes.position = 0;
    bytes.size = 20;
    gif = DGifOpen(&bytes, read_bytes, &error);
    if (gif)
    {
        assert(DGifSlurp(gif) == GIF_ERROR);
        DGifCloseFile(gif, &error);
    }
    bytes.position = bytes.size = 0;
    assert(DGifOpen(&bytes, read_bytes, &error) == 0);
    return 0;
}
