/* Explicit kerning consistently controls wrapping, right alignment, geometry and owned path lifetime. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
    FVM_TextLayout *layout=(FVM_TextLayout*)1;
    assert(fvm_text_layout_create_configured(0,"AV",2,0,2,&layout)==FVM_INVALID_ARGUMENT && !layout);
#ifdef FVM_HAS_FONTS
    {
        FVM_Font *font=0;FVM_TextLayout *enabled=0,*disabled=0;
        FVM_TextLayoutOptions options={46,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};
        FVM_TextLayoutMetrics on,off;FVM_TextLayoutGlyph a,b;FVM_Path *path=0;FVM_PathBounds box;
        assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
        assert(fvm_text_layout_create_configured(font,"AV",2,&options,0,&enabled)==FVM_OK);
        assert(fvm_text_layout_create_configured(font,"AV",2,&options,FVM_FONT_SHAPE_NO_KERNING,&disabled)==FVM_OK);
        assert(fvm_text_layout_metrics(enabled,&on)==FVM_OK && fvm_text_layout_metrics(disabled,&off)==FVM_OK);
        assert(on.line_count==1 && off.line_count==2 && off.glyph_count==2);
        fvm_text_layout_destroy(enabled);fvm_text_layout_destroy(disabled);
        options.max_width=100;options.alignment=FVM_TEXT_ALIGN_RIGHT;
        assert(fvm_text_layout_create_configured(font,"AV",2,&options,0,&enabled)==FVM_OK);
        assert(fvm_text_layout_create_configured(font,"AV",2,&options,FVM_FONT_SHAPE_NO_KERNING,&disabled)==FVM_OK);
        assert(fvm_text_layout_glyph(enabled,0,&a)==FVM_OK && fvm_text_layout_glyph(disabled,0,&b)==FVM_OK && fabs(a.x-b.x-3.2)<0.02 && a.shaped.cluster==b.shaped.cluster);
        assert(fvm_text_layout_path(disabled,&path)==FVM_OK);
        fvm_font_destroy(font);fvm_text_layout_destroy(enabled);fvm_text_layout_destroy(disabled);
        assert(fvm_path_bounds(path,&box)==FVM_OK);fvm_path_destroy(path);
    }
#else
    assert(fvm_text_layout_create_configured(0,"AV",2,0,FVM_FONT_SHAPE_NO_KERNING,&layout)==FVM_UNSUPPORTED && !layout);
#endif
    return 0;
}
