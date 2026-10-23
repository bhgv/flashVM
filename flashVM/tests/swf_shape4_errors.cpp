#include "swf_shape4_fixture.hpp"
static void rejected(const Bytes &bytes, FVM_Result expected)
{
    FVM_SWFShape decoded; decoded.id=777; decoded.bounds.x_min=42;
    FVM_SWFShapePaint prior; prior.rgb=123; decoded.paints.push_back(prior);
    assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,decoded)==expected);
    assert(decoded.id==777 && decoded.bounds.x_min==42 && decoded.paints.size()==1 && decoded.paints[0].rgb==123);
}
int main()
{
    for(unsigned int kind=0;kind<3;++kind) {
        Bytes source=shape4(kind==0?0x10:kind==1?0x12:0x13,2,1,-255);
        for(size_t n=0;n<source.size();++n) { Bytes short_record; for(size_t i=0;i<n;++i) short_record.push_back(source[i]); rejected(short_record,FVM_INVALID_ARGUMENT); }
        Bytes extra=source; extra.push_back(0); rejected(extra,FVM_INVALID_ARGUMENT);
    }
    size_t header=0,flags=0,lines=0; Bytes source=shape4(0x13,0,0,0,2,&header,&flags,&lines);
    for(unsigned int bit=2;bit<8;++bit) { Bytes bad=source; bad[flags]=(unsigned char)(1U<<bit); rejected(bad,FVM_INVALID_ARGUMENT); }
    for(unsigned int mode=0;mode<256;++mode) {
        if((mode&15)==2 && (mode>>6)<3 && ((mode>>4)&3)<2) continue;
        if((mode&15)!=2 && (mode&15)!=0) continue;
        Bytes bad=source; bad[header]=(unsigned char)mode; rejected(bad,FVM_INVALID_ARGUMENT);
    }
    Bytes bad=source; bad[header+1]=200; bad[header+6]=100; rejected(bad,FVM_INVALID_ARGUMENT);
    bad=source; bad[lines]=1; rejected(bad,FVM_INVALID_ARGUMENT);
    const int focal[]={-32768,-257,-256,256,257,32767};
    for(unsigned int i=0;i<sizeof(focal)/sizeof(focal[0]);++i) rejected(shape4(0x13,0,0,focal[i]),FVM_UNSUPPORTED);
    for(unsigned int tag_code=2;tag_code<=32;tag_code+=tag_code==2?20:10) {
        Bytes legacy=gradient_shape(0x10,tag_code,&header); legacy[header]=0x42; FVM_SWFShape out;
        assert(fvm_swf_read_shape(legacy.begin(),legacy.size(),tag_code,out)==FVM_INVALID_ARGUMENT);
    }
    return 0;
}
