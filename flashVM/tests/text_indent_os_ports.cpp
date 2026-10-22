/* Independent font owners inspect first/continuation offsets and retained outlines on OS workers. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"
#include <cassert>
#ifdef FVM_HAS_FONTS
struct IndentJob {double indent;unsigned passes;};
static void indent_worker(void *raw)
{
 IndentJob *job=(IndentJob*)raw;FVM_Font *font=0;assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);FVM_TextLayoutOptions o={102,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};
 for(unsigned i=0;i<64;++i){FVM_TextLayout *layout=0;FVM_TextLayoutLine a,b;FVM_TextLayoutMetrics m;assert(fvm_text_layout_create_indented(font,"A V A V",7,&o,FVM_FONT_SHAPE_NO_KERNING,0,job->indent,&layout)==FVM_OK && fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2);assert(fvm_text_layout_line(layout,0,&a)==FVM_OK && a.x==job->indent && fvm_text_layout_line(layout,1,&b)==FVM_OK && b.x==0);assert(a.advance==(job->indent>0?72:108) && b.advance==(job->indent>0?60:24));FVM_Path *p=0;assert(fvm_text_layout_path(layout,&p)==FVM_OK);fvm_text_layout_destroy(layout);assert(fvm_path_count(p)>0);fvm_path_destroy(p);++job->passes;}fvm_font_destroy(font);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
 IndentJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].indent=i%2?-18:30;jobs[i].passes=0;threads[i]=fvm_port_thread_start(indent_worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].passes==64);}
#endif
 return 0;
}
