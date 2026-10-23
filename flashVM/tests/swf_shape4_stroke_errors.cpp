#include "swf_shape4_stroke_fixture.hpp"
static void reject(const Bytes &bytes,FVM_Result expected)
{
    FVM_SWFShape out; out.id=777; FVM_SWFShapePaint prior; prior.width=123; prior.no_close=true; out.paints.push_back(prior);
    assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==expected);
    assert(out.id==777 && out.paints.size()==1 && out.paints[0].width==123 && out.paints[0].no_close);
}
int main()
{
    for(unsigned k=0;k<4;++k) {
        unsigned kind=k==0?0:k==1?0x10:k==2?0x12:0x13; Bytes source=shape4_stroke(kind,2,1,-255,80,15,8,4,true);
        for(size_t n=0;n<source.size();++n) { Bytes short_record; for(size_t i=0;i<n;++i) short_record.push_back(source[i]); reject(short_record,FVM_INVALID_ARGUMENT); }
        Bytes extra=source; extra.push_back(0); reject(extra,FVM_INVALID_ARGUMENT);
    }
    const int focal[]={-32768,-257,-256,256,257,32767};
    for(unsigned i=0;i<6;++i) reject(shape4_stroke(0x13,0,0,focal[i]),FVM_UNSUPPORTED);
    /* Nested gradients retain their own reserved mode/count and ordering checks. */
    size_t style=0; Bytes source=shape4_stroke(0x13,0,0,0,80,2,8,0,false,&style);
    size_t fill_header=0,fill_flags=0; Bytes plain=shape4(0x13,0,0,0,2,&fill_header,&fill_flags); (void)plain;
    /* Locate the nested header from the independently authored fill record. */
    size_t header=style+4+(fill_header-fill_flags-2);
    for(unsigned bad=0;bad<3;++bad) { Bytes bytes=source; bytes[header]=(unsigned char)(bad==0?0:bad==1?0xc2:0x22); reject(bytes,FVM_INVALID_ARGUMENT); }
    Bytes bytes=source; bytes[header+1]=200; bytes[header+6]=100; reject(bytes,FVM_INVALID_ARGUMENT);
    /* A complete bitmap FillType is rejected before backend-dependent graph construction. */
    bytes=source; bytes.resize(style+4); bytes.push_back(0x40); u16(bytes,7); bytes.push_back(0);
    reject(bytes,FVM_UNSUPPORTED);
    return 0;
}
