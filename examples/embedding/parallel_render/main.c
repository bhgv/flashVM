#include "flashvm/render_jobs.h"
#include <stdio.h>
/* Each worker owns a different pixel. A real renderer would use immutable
 * MovieClip display snapshots, then composite layers in display-list order. */
static void draw(void *buffer, unsigned int pixel) {
    unsigned char *rgba=(unsigned char *)buffer;
    rgba[pixel*4]=255;
    rgba[pixel*4+1]=(unsigned char)(pixel&255);
    rgba[pixel*4+2]=0;
    rgba[pixel*4+3]=255;
}
int main(void) {
    unsigned char pixels[64*4];
    if (fvm_render_jobs_run(64,4,draw,pixels)!=0) return 1;
    printf("RGBA pixel 0: %u %u %u %u\n",
        (unsigned int)pixels[0],(unsigned int)pixels[1],
        (unsigned int)pixels[2],(unsigned int)pixels[3]);
    return 0;
}
