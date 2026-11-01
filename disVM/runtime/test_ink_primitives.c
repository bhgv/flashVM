/* Independent native buffer/bitmap invariants and an optional timing report. */
#include "editor_ink.h"
#include "editor_stamp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/* Fail explicitly without disabling expressions under NDEBUG. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "FAIL ink primitive: %s\n", label);
        exit(1);
    }
}


/* Count actual callbacks rather than infer retained slot state from its token. */
static int count(void *user, const FVM_Path *path, FVM_RasterColor color, double width)
{
    unsigned int *total = user;
    require(path && fvm_path_count(path) >= 2 && color.a == 255 && width > 0, "copied preview geometry");
    (*total)++;
    return 1;
}


/* Independent opacity/pixel probes and registry retirement with a stale-stream
 * negative control. Timing is reported, never used as a flaky pass threshold. */
int main(void)
{
    unsigned long token;
    unsigned int index;
    unsigned int total = 0;
    FVM_Affine identity = {1, 0, 0, 1, 0, 0};
    FVM_Path *path = NULL;
    FVM_PathCommand command;
    unsigned char pixels[256 * 256 * 4];
    FVM_RasterSurface surface = {pixels, 256, 256, 256 * 4};
    FVM_RasterColor blue = {20, 70, 190, 255};
    clock_t start;
    double stamp_time;
    double vector_time;
    token = dvm_ink_begin(1, 0x1446be, 8, 100100);
    require(token && !dvm_ink_read(token)[0], "live arrays cannot leave native owner");
    require(dvm_ink_point(token, 200200) && dvm_ink_end(token, 300300), "seal native trajectory");
    require(strlen(dvm_ink_read(token)) == 24 && !dvm_ink_point(token, 400400), "frozen immutable worker input");
    dvm_ink_ack(token, "new vector", 10);
    dvm_ink_commit(1, "old vector", 10);
    require(dvm_ink_preview(1, &identity, 1, count, &total) && total == 1, "stale vector stream keeps preview");
    dvm_ink_commit(1, "new vector", 10);
    total = 0;
    require(dvm_ink_preview(1, &identity, 1, count, &total) && total == 0,
            "matching publication atomically retires preview");
    token = dvm_ink_begin(1, 0x1446be, 8, 0);
    for (index = 1; index < DVM_INK_POINTS; ++index)
    {
        require(dvm_ink_point(token, index), "bounded buffer prefix retention");
    }
    require(!dvm_ink_point(token, 5000) && dvm_ink_end(token, 5000) &&
                strlen(dvm_ink_read(token)) == DVM_INK_POINTS * 8,
            "full-buffer release preserves earlier raw ink");
    dvm_ink_reset();
    require(fvm_path_create(&path) == FVM_OK, "path allocation");
    memset(&command, 0, sizeof(command));
    for (index = 0; index < 1024; ++index)
    {
        command.verb = index ? FVM_PATH_LINE : FVM_PATH_MOVE;
        command.x[0] = 16 + index % 224;
        command.y[0] = 128 + sin(index * 0.03) * 90;
        require(fvm_path_add(path, &command) == FVM_OK, "raw native trajectory");
    }
    memset(pixels, 0, sizeof(pixels));
    require(dvm_editor_stamp_draw(path, &surface, blue, 8, 0) == FVM_OK, "native stamp");
    require(pixels[(128 * 256 + 16) * 4 + 2] == 190 && pixels[(128 * 256 + 16) * 4 + 3] == 255,
            "opaque interior and initial round cap");
    require(!pixels[3], "untouched outside ink bounds");
    start = clock();
    for (index = 0; index < 10; ++index)
    {
        memset(pixels, 255, sizeof(pixels));
        require(dvm_editor_stamp_draw(path, &surface, blue, 8, 0) == FVM_OK, "timed stamp");
    }
    stamp_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    start = clock();
    for (index = 0; index < 10; ++index)
    {
        memset(pixels, 255, sizeof(pixels));
        require(fvm_path_draw(path, &surface, blue, 8, 0) == FVM_OK, "reference vector raster");
    }
    vector_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    fvm_path_destroy(path);
    printf(
        "PASS native ink primitives: sealed buffers, publication fence, capacity, stamp pixels;10x1024 stamp=%.6fs vector=%.6fs\n",
        stamp_time,
        vector_time);
    return 0;
}
