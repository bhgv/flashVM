/* Analytical geometric-font tracking oracle, independent of system fonts. */
#include "flashvm/text_layout.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <math.h>
int main(void)
{
    FVM_TextLayout *layout=(FVM_TextLayout*)1;
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,0,4097,&layout)==FVM_INVALID_ARGUMENT && !layout);
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,0,-4097,&layout)==FVM_INVALID_ARGUMENT && !layout);
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,2,0,&layout)==FVM_INVALID_ARGUMENT && !layout);
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,0,HUGE_VAL,&layout)==FVM_INVALID_ARGUMENT && !layout);
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,0,0,0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
    {
        FVM_Font *font=0;FVM_TextLayoutMetrics m;FVM_TextLayoutGlyph g;
        FVM_TextLayoutOptions o={0,0,FVM_TEXT_LTR,FVM_TEXT_ALIGN_LEFT,0};
        FVM_Path *path=0;FVM_PathBounds box;
        assert(fvm_font_read(fvm_test_font,sizeof(fvm_test_font),0,40,&font)==FVM_OK);
        assert(fvm_text_layout_create_spaced(font,"AV",2,&o,FVM_FONT_SHAPE_NO_KERNING,2.5,&layout)==FVM_OK);
        assert(fvm_text_layout_metrics(layout,&m)==FVM_OK && fabs(m.advance_width-53)<0.02);
        assert(fvm_text_layout_glyph(layout,1,&g)==FVM_OK && fabs(g.x-26.5)<0.02 && fabs(g.shaped.x_advance-26.5)<0.02);
        assert(fvm_text_layout_path(layout,&path)==FVM_OK);fvm_text_layout_destroy(layout);
        o.max_width=50;
        assert(fvm_text_layout_create_spaced(font,"AV",2,&o,FVM_FONT_SHAPE_NO_KERNING,2.5,&layout)==FVM_OK);
        assert(fvm_text_layout_metrics(layout,&m)==FVM_OK && m.line_count==2);fvm_text_layout_destroy(layout);
        o.max_width=0;
        assert(fvm_text_layout_create_spaced(font,"AV",2,&o,FVM_FONT_SHAPE_NO_KERNING,-2,&layout)==FVM_OK);
        assert(fvm_text_layout_metrics(layout,&m)==FVM_OK && fabs(m.advance_width-44)<0.02);fvm_text_layout_destroy(layout);
        assert(fvm_text_layout_create_spaced(font,"AV",2,&o,FVM_FONT_SHAPE_NO_KERNING,-25,&layout)==FVM_UNSUPPORTED && !layout);
        assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,1,&layout)==FVM_UNSUPPORTED && !layout);
        assert(fvm_text_layout_create_spaced(font,"fi",2,&o,0,0,&layout)==FVM_OK);fvm_text_layout_destroy(layout);
        fvm_font_destroy(font);assert(fvm_path_bounds(path,&box)==FVM_OK);fvm_path_destroy(path);
    }
#else
    assert(fvm_text_layout_create_spaced(0,"AV",2,0,0,1,&layout)==FVM_UNSUPPORTED && !layout);
#endif
    return 0;
}
