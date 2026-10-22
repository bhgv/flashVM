/* Independent font owners construct and inspect justified layouts on real OS workers. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"
#include <cassert>
#include <cmath>
#ifdef FVM_HAS_FONTS
struct JustifyJob { double width,spacing;unsigned passes;bool failed; };
static void justify_worker(void *raw)
{
 JustifyJob *job=(JustifyJob*)raw;FVM_Font *font=0;if(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)!=FVM_OK){job->failed=true;return;}
 FVM_TextLayoutOptions o={job->width,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_JUSTIFY,0};
 for(unsigned i=0;i<64;++i){FVM_TextLayout *layout=0;FVM_TextLayoutMetrics m;FVM_TextLayoutGlyph g;
  if(fvm_text_layout_create_spaced(font,"A V A V",7,&o,FVM_FONT_SHAPE_NO_KERNING,job->spacing,&layout)!=FVM_OK){job->failed=true;break;}
  if(fvm_text_layout_metrics(layout,&m)!=FVM_OK || m.line_count!=2 || std::fabs(m.advance_width-job->width)>1e-9 || fvm_text_layout_glyph(layout,2,&g)!=FVM_OK || std::fabs(g.x-(job->width-24-job->spacing))>1e-9)job->failed=true;
  FVM_Path *path=0;if(fvm_text_layout_path(layout,&path)!=FVM_OK)job->failed=true;fvm_text_layout_destroy(layout);if(!path || !fvm_path_count(path))job->failed=true;fvm_path_destroy(path);if(job->failed)break;++job->passes;
 }fvm_font_destroy(font);
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
 JustifyJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].width=i%2?94:90;jobs[i].spacing=i%2?0.1:-0.2;jobs[i].passes=0;jobs[i].failed=false;threads[i]=fvm_port_thread_start(justify_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].passes==64);}
#endif
 return 0;
}
