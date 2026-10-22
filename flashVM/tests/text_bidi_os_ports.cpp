/* Independent native OS workers verify RTL/LTR maps without a shared VM or font. */
#include "flashvm/font.h"
#include "../ports/thread_port.h"
#include <cassert>
#ifdef FVM_HAS_FONTS
struct BidiJob { bool rtl; bool failed; unsigned passes; };
/* Retain all input and output memory on this worker's own stack. */
static void bidi_worker(void *raw)
{
    BidiJob *job=(BidiJob*)raw;
    for(unsigned pass=0;pass<256;++pass)
    {
        unsigned int logical[4]={0x5d0,0x5d1,32,65};
        unsigned int visual[4];size_t map[4];unsigned char levels[4];
        FVM_TextDirection direction=job->rtl?FVM_TEXT_RTL:FVM_TEXT_LTR;
        if(fvm_text_bidi(logical,4,&direction,visual,map,levels)!=FVM_OK)
        {job->failed=true;return;}
        const size_t rtl_map[4]={3,2,1,0},ltr_map[4]={1,0,2,3};
        const unsigned char rtl_levels[4]={1,1,1,2},ltr_levels[4]={1,1,0,0};
        for(unsigned i=0;i<4;++i)
            if(map[i]!=(job->rtl?rtl_map[i]:ltr_map[i]) ||
               visual[i]!=logical[map[i]] || levels[i]!=(job->rtl?rtl_levels[i]:ltr_levels[i]))
            {job->failed=true;return;}
        ++job->passes;
    }
}
#endif
/* Joining establishes completion before the parent reads worker-owned results. */
int main()
{
#ifdef FVM_HAS_FONTS
    BidiJob jobs[4];FVM_PortThread *threads[4];
    for(unsigned i=0;i<4;++i)
    {
        jobs[i].rtl=i%2!=0;jobs[i].failed=false;jobs[i].passes=0;
        threads[i]=fvm_port_thread_start(bidi_worker,jobs+i);assert(threads[i]);
    }
    for(unsigned i=0;i<4;++i)
    {fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].passes==256);}
#endif
    return 0;
}
