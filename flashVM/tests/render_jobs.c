#include "flashvm/render_jobs.h"
#include <assert.h>
#define COUNT 257
static void render(void *data, unsigned int index) {
    unsigned int *outputs=(unsigned int *)data;
    outputs[index]=index+1;
}
int main(void) {
    unsigned int outputs[COUNT];
    unsigned int i;
    for (i=0;i<COUNT;++i) outputs[i]=0;
    assert(fvm_render_jobs_run(COUNT,4,render,outputs)==0);
    for (i=0;i<COUNT;++i) assert(outputs[i]==i+1);
    assert(fvm_render_jobs_run(COUNT,0,render,outputs)==0);
    assert(fvm_render_jobs_run(0,4,render,outputs)==0);
    assert(fvm_render_jobs_run(1,1,0,outputs)==-1);
    return 0;
}
