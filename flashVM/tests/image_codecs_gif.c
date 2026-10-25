#include <stdio.h>
#include <stdlib.h>
#include "gif_lib.h"
#include "image_codecs_memory.h"


static int write_bytes(GifFileType *gif, const GifByteType *bytes, int count)
{
    TestImageBytes *out = (TestImageBytes *)gif->UserData;
    if (count < 0 || (size_t)count > sizeof(out->data) - out->size)
    {
        return 0;
    }
    memcpy(out->data + out->size, bytes, (size_t)count);
    out->size += (size_t)count;
    return count;
}


static int read_bytes(GifFileType *gif, GifByteType *bytes, int count)
{
    TestImageBytes *input = (TestImageBytes *)gif->UserData;
    size_t available = input->size - input->position;
    if (count < 0)
    {
        return 0;
    }
    if ((size_t)count > available)
    {
        count = (int)available;
    }
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
    if (!(palette))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gif = EGifOpen(&bytes, write_bytes, &error);
    if (!(gif))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    EGifSetGifVersion(gif, 1);
    if (!(EGifPutScreenDesc(gif, 8, 8, 8, 0, palette) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    control.DisposalMode = DISPOSE_BACKGROUND;
    control.UserInputFlag = 0;
    control.DelayTime = 12;
    control.TransparentColor = 0;
    if (!(EGifGCBToExtension(&control, extension) == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, 4, extension) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(EGifPutImageDesc(gif, 0, 0, 8, 8, 0, 0) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            row[x] = (GifPixelType)((x + y) % 2);
        }
        if (!(EGifPutLine(gif, row, 8) == GIF_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* A second frame tests animation and a local color map. */
    if (!(EGifPutImageDesc(gif, 2, 3, 2, 2, 0, palette) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    row[0] = row[1] = 1;
    if (!(EGifPutLine(gif, row, 2) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(EGifPutLine(gif, row, 2) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(EGifCloseFile(gif, &error) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GifFreeMapObject(palette);
    if (!(bytes.size > 20 && memcmp(bytes.data, "GIF89a", 6) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    gif = DGifOpen(&bytes, read_bytes, &error);
    if (!(gif && DGifSlurp(gif) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gif->SWidth == 8 && gif->SHeight == 8 && gif->ImageCount == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            if (!(gif->SavedImages[0].RasterBits[y * 8 + x] == (x + y) % 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(DGifSavedExtensionToGCB(gif, 0, &read_control) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read_control.TransparentColor == 0 && read_control.DelayTime == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(read_control.DisposalMode == DISPOSE_BACKGROUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gif->SavedImages[1].ImageDesc.ColorMap->Colors[1].Red == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(gif->SavedImages[1].ImageDesc.Left == 2 && gif->SavedImages[1].ImageDesc.Top == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(DGifCloseFile(gif, &error) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bytes.position = 0;
    bytes.size = 20;
    gif = DGifOpen(&bytes, read_bytes, &error);
    if (gif)
    {
        if (!(DGifSlurp(gif) == GIF_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        DGifCloseFile(gif, &error);
    }
    bytes.position = bytes.size = 0;
    if (!(DGifOpen(&bytes, read_bytes, &error) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
