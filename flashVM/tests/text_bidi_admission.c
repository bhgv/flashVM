/* C89 hostile enum/UTF-32 admission and failure-atomic output contracts. */
#include "flashvm/font.h"
#include <assert.h>
#include <string.h>
static void rejected(unsigned int *input, size_t count, FVM_TextDirection *direction)
{
    unsigned int visual[3] = {91,92,93};
    size_t map[3] = {81,82,83};
    unsigned char levels[3] = {71,72,73};
    unsigned char before[sizeof(FVM_TextDirection)];
    memcpy(before,direction,sizeof(before));
    assert(fvm_text_bidi(input,count,direction,visual,map,levels)==FVM_INVALID_ARGUMENT);
    assert(visual[0]==91 && visual[1]==92 && visual[2]==93);
    assert(map[0]==81 && map[1]==82 && map[2]==83);
    assert(levels[0]==71 && levels[1]==72 && levels[2]==73);
    assert(!memcmp(before,direction,sizeof(before)));
}
int main(void)
{
    unsigned int input[3]={65,0x5d0,0x5d1},visual[3];
    size_t map[3];unsigned char levels[3];
    FVM_TextDirection direction=FVM_TEXT_AUTO;
    rejected(input,1000001,&direction);
    input[1]=0xd800;rejected(input,3,&direction);
    input[1]=0xdfff;rejected(input,3,&direction);
    input[1]=0x110000;rejected(input,3,&direction);
    input[1]=0x5d0;
    memset(&direction,255,sizeof(direction));rejected(input,3,&direction);
    rejected(0,0,&direction);
    direction=FVM_TEXT_AUTO;
    assert(fvm_text_bidi(input,3,0,visual,map,levels)==FVM_INVALID_ARGUMENT);
    assert(fvm_text_bidi(0,3,&direction,visual,map,levels)==FVM_INVALID_ARGUMENT);
    assert(fvm_text_bidi(input,3,&direction,0,map,levels)==FVM_INVALID_ARGUMENT);
    assert(fvm_text_bidi(input,3,&direction,visual,0,levels)==FVM_INVALID_ARGUMENT);
    assert(fvm_text_bidi(input,3,&direction,visual,map,0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
    assert(fvm_text_bidi(0,0,&direction,0,0,0)==FVM_OK && direction==FVM_TEXT_LTR);
    direction=FVM_TEXT_RTL;
    assert(fvm_text_bidi(0,0,&direction,0,0,0)==FVM_OK && direction==FVM_TEXT_RTL);
    direction=FVM_TEXT_AUTO;
    assert(fvm_text_bidi(input,3,&direction,visual,map,levels)==FVM_OK);
    assert(direction==FVM_TEXT_LTR && visual[1]==0x5d1 && map[1]==2);
#else
    assert(fvm_text_bidi(input,3,&direction,visual,map,levels)==FVM_UNSUPPORTED);
    assert(direction==FVM_TEXT_AUTO);
#endif
    return 0;
}
