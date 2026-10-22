/* Four real OS workers derive periodic intervals from independently owned font faces. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"
#include <cassert>
#ifdef FVM_HAS_FONTS
struct TabsJob {double size;unsigned passes;};
static void tabs_worker(void *raw)
{TabsJob *job=(TabsJob*)raw;FVM_Font *font=0;assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,job->size,&font)==FVM_OK);FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};for(unsigned i=0;i<64;++i){double average=0;assert(fvm_font_average_width(font,&average)==FVM_OK);double period=average*4;FVM_TextLayout *l=0;FVM_TextLayoutMetrics m;FVM_TextLayoutGlyph g;assert(fvm_text_layout_create_periodic(font,"A\tV\tA",5,&o,FVM_FONT_SHAPE_NO_KERNING,0.5,10,period,&l)==FVM_OK);assert(fvm_text_layout_metrics(l,&m)==FVM_OK && m.advance_width==period*2+job->size*0.6+0.5 && fvm_text_layout_glyph(l,4,&g)==FVM_OK && g.x==period*2);FVM_Path *p=0;assert(fvm_text_layout_path(l,&p)==FVM_OK);fvm_text_layout_destroy(l);assert(fvm_path_count(p)>0);fvm_path_destroy(p);++job->passes;}fvm_font_destroy(font);}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
 TabsJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].size=i%2?20:40;jobs[i].passes=0;threads[i]=fvm_port_thread_start(tabs_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].passes==64);}
#endif
 return 0;
}
