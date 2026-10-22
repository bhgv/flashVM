/* Immutable layout readers export independent line bars concurrently after font destruction. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include "../ports/thread_port.h"
#include <cassert>
#include <cmath>
#ifdef FVM_HAS_FONTS
struct UnderlineJob { const FVM_TextLayout *layout;size_t line;double center;bool failed;unsigned passes; };
/* Share read-only layout data; every path and result buffer is worker-owned. */
static void underline_worker(void *raw)
{
 UnderlineJob *job=(UnderlineJob*)raw;
 for(unsigned pass=0;pass<128;++pass){
  FVM_TextLayoutLine line;FVM_Path *path=0;FVM_PathBounds bounds;
  if(fvm_text_layout_line(job->layout,job->line,&line)!=FVM_OK ||
     fvm_text_layout_line_underline_path(job->layout,job->line,job->center,2,&path)!=FVM_OK)
  {job->failed=true;return;}
  bool valid=fvm_path_count(path)==5 && fvm_path_bounds(path,&bounds)==FVM_OK &&
    std::fabs(bounds.x_min-line.x)<0.02 && std::fabs(bounds.x_max-(line.x+line.advance))<0.02 &&
    std::fabs(bounds.y_min-(line.baseline+job->center-1))<0.02 && std::fabs(bounds.y_max-(line.baseline+job->center+1))<0.02;
  fvm_path_destroy(path);if(!valid){job->failed=true;return;}++job->passes;
 }
}
#endif
/* Join every reader before releasing the single immutable shared layout. */
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Font *font=0;FVM_TextLayout *layout=0;FVM_FontUnderlineMetrics metrics;
 FVM_TextLayoutOptions options={100,48,FVM_TEXT_AUTO,FVM_TEXT_ALIGN_RIGHT,0};
 assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK && fvm_font_underline_metrics(font,&metrics)==FVM_OK);
 assert(fvm_text_layout_create_spaced(font,"AV\nA",4,&options,FVM_FONT_SHAPE_NO_KERNING,2,&layout)==FVM_OK);fvm_font_destroy(font);
 UnderlineJob jobs[4];FVM_PortThread *threads[4];
 for(unsigned i=0;i<4;++i){jobs[i].layout=layout;jobs[i].line=i%2;jobs[i].center=metrics.center_y+i;jobs[i].failed=false;jobs[i].passes=0;threads[i]=fvm_port_thread_start(underline_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].passes==128);}
 fvm_text_layout_destroy(layout);
#endif
 return 0;
}
