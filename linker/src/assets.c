/* Checked resource import, character allocation and display/sound events. */
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct { Buffer *buffer; unsigned used; } Bits;
static void bits(Bits *b, uint32_t value, unsigned count)
{
    unsigned i;
    for(i=count;i>0;--i) {
        if(!b->used) swl_byte(b->buffer,0);
        if(b->buffer->failed) return;
        b->buffer->data[b->buffer->size-1]|=(unsigned char)(((value>>(i-1))&1)<<(7-b->used));
        b->used=(b->used+1)%8;
    }
}
static unsigned width(int32_t value)
{
    unsigned n=2;
    while(n<31 && (value<-(INT32_C(1)<<(n-1)) || value>=(INT32_C(1)<<(n-1)))) ++n;
    return n;
}
static void edge(Bits *b, int32_t delta, int vertical)
{
    unsigned n=width(delta);
    bits(b,1,1); bits(b,1,1); bits(b,n-2,4); bits(b,0,1); bits(b,(unsigned)vertical,1); bits(b,(uint32_t)delta,n);
}
/* DefineShape3, one fill, four axis-aligned edges; no host structure layout. */
static void shape(Buffer *out, unsigned id, unsigned w, unsigned h, uint32_t rgba, unsigned bitmap)
{
    Buffer b={0}; Bits stream; unsigned i;
    swl_word(&b,id); swl_rect(&b,w,h); swl_byte(&b,1);
    if(bitmap) {
        swl_byte(&b,0x41); swl_word(&b,bitmap); stream.buffer=&b; stream.used=0;
        bits(&stream,1,1); bits(&stream,22,5); bits(&stream,20u<<16,22); bits(&stream,20u<<16,22);
        bits(&stream,0,1); bits(&stream,1,5); bits(&stream,0,1); bits(&stream,0,1);
    } else {
        swl_byte(&b,0); for(i=0;i<4;++i) swl_byte(&b,rgba>>(24-i*8));
    }
    swl_byte(&b,0); /* no line styles */
    swl_byte(&b,0x10); /* one fill-index bit, zero line-index bits */
    stream.buffer=&b; stream.used=0;
    bits(&stream,0,1); bits(&stream,5,5); /* move and FillStyle1 */
    bits(&stream,1,5); bits(&stream,0,1); bits(&stream,0,1); bits(&stream,1,1);
    edge(&stream,(int32_t)w*20,0); edge(&stream,(int32_t)h*20,1);
    edge(&stream,-(int32_t)w*20,0); edge(&stream,-(int32_t)h*20,1);
    bits(&stream,0,6);
    if(b.failed) out->failed=1; else swl_tag(out,32,b.data,b.size);
    free(b.data);
}
static int available(SwlContext *c, const char *name, char *e, size_t cap)
{
    size_t i,n;
    if(!c || !name || !(n=strlen(name)) || n>255) return swl_fail(e,cap,"invalid asset name/context");
    for(i=0;i<n;++i) {
        unsigned ch=(unsigned char)name[i];
        if(!((ch>='A' && ch<='Z') || (ch>='a' && ch<='z') || ch=='_' ||
            (i && ((ch>='0' && ch<='9') || ch=='.')))) return swl_fail(e,cap,"invalid asset name");
    }
    for(i=0;i<c->asset_count;++i) if(!strcmp(c->assets[i].item.name,name)) return swl_fail(e,cap,"duplicate asset name");
    if(c->asset_count==SWL_ITEMS) return swl_fail(e,cap,"asset count limit exceeded");
    return 1;
}
static int save(SwlContext *c, const char *name, Buffer *b, unsigned id, int displayable, char *e, size_t cap)
{
    Asset *asset;
    if(b->failed || b->size>SWL_LIMIT-c->bytes) { free(b->data); return swl_fail(e,cap,"asset size/allocation limit exceeded"); }
    asset=&c->assets[c->asset_count++]; strcpy(asset->item.name,name);
    asset->item.data=b->data; asset->item.size=b->size; asset->id=id; asset->displayable=displayable;
    c->bytes+=b->size; return 1;
}
int swl_add_rect(SwlContext *c, const char *name, unsigned w, unsigned h, uint32_t rgba, char *e, size_t cap)
{
    Buffer b={0}; unsigned id;
    if(!available(c,name,e,cap)) return 0;
    if(!w || w>3276 || !h || h>3276) return swl_fail(e,cap,"rectangle dimensions outside 1..3276 pixels");
    id=(unsigned)c->asset_count*2+1; shape(&b,id,w,h,rgba,0);
    return save(c,name,&b,id,1,e,cap);
}
int swl_add_svf(SwlContext *c, const char *name, const void *data, size_t n, char *e, size_t cap)
{
    char text[256], *tokens[6], *cursor; unsigned values[2]={0,0},rgba=0,i,k; size_t count=0;
    if(!data || !n || n>=sizeof(text) || memchr(data,0,n)) return swl_fail(e,cap,"invalid SVF1 text");
    memcpy(text,data,n); text[n]=0; cursor=text;
    while(*cursor) {
        while(*cursor==' ' || *cursor=='\t' || *cursor=='\r' || *cursor=='\n') ++cursor;
        if(!*cursor) break;
        if(count==6) return swl_fail(e,cap,"trailing SVF1 token");
        tokens[count++]=cursor;
        while(*cursor && *cursor!=' ' && *cursor!='\t' && *cursor!='\r' && *cursor!='\n') ++cursor;
        if(*cursor) *cursor++=0;
    }
    if(count!=6 || strcmp(tokens[0],"SVF") || strcmp(tokens[1],"1") || strcmp(tokens[2],"rect") || strlen(tokens[5])!=8)
        return swl_fail(e,cap,"expected SVF 1 rect WIDTH HEIGHT RRGGBBAA");
    for(k=0;k<2;++k) for(i=0;tokens[3+k][i];++i) {
        unsigned ch=(unsigned char)tokens[3+k][i];
        if(ch<'0' || ch>'9' || values[k]>3276/10 || values[k]*10+ch-'0'>3276)
            return swl_fail(e,cap,"SVF1 dimension outside 1..3276");
        values[k]=values[k]*10+ch-'0';
    }
    for(i=0;i<8;++i) {
        unsigned ch=(unsigned char)tokens[5][i],v;
        if(ch>='0' && ch<='9') v=ch-'0'; else if(ch>='A' && ch<='F') v=ch-'A'+10;
        else if(ch>='a' && ch<='f') v=ch-'a'+10; else return swl_fail(e,cap,"invalid SVF1 RGBA");
        rgba=rgba*16+v;
    }
    return swl_add_rect(c,name,values[0],values[1],(uint32_t)rgba,e,cap);
}
/* Baseline 8-bit JPEG header validation. Entropy data is preserved, not decoded. */
int swl_add_jpeg(SwlContext *c, const char *name, const void *data, size_t n, char *e, size_t cap)
{
    const unsigned char *p=(const unsigned char *)data; size_t pos=2,len; unsigned w=0,h=0,id; int scan=0;
    Buffer b={0},body={0};
    if(!available(c,name,e,cap)) return 0;
    if(!p || n<4 || n>16u*1024u*1024u || p[0]!=255 || p[1]!=216 || p[n-2]!=255 || p[n-1]!=217) goto bad;
    while(pos<n-2) {
        unsigned marker;
        if(p[pos++]!=255) goto bad;
        while(pos<n && p[pos]==255) ++pos;
        if(pos>=n) goto bad;
        marker=p[pos++];
        if(n-pos<2) goto bad;
        len=(size_t)p[pos]*256+p[pos+1];
        if(len<2 || len>n-pos) goto bad;
        if(marker==192) {
            if(len<8 || p[pos+2]!=8 || (p[pos+7]!=1 && p[pos+7]!=3) || len!=8u+3u*p[pos+7]) goto bad;
            h=(unsigned)p[pos+3]*256+p[pos+4]; w=(unsigned)p[pos+5]*256+p[pos+6];
        } else if(marker>=193 && marker<=207 && marker!=196) goto bad;
        if(marker==218) { if(!w || !h || len<6) goto bad; scan=1; break; }
        pos+=len;
    }
    if(!scan || w>3276 || h>3276) goto bad;
    id=(unsigned)c->asset_count*2+1;
    swl_word(&body,id); swl_bytes(&body,p,n);
    if(body.failed) b.failed=1; else swl_tag(&b,21,body.data,body.size);
    free(body.data); shape(&b,id+1,w,h,0,id);
    return save(c,name,&b,id+1,1,e,cap);
bad: return swl_fail(e,cap,"expected bounded baseline 8-bit JPEG with SOF0/SOS/EOI");
}
int swl_add_wav(SwlContext *c, const char *name, const void *data, size_t n, char *e, size_t cap)
{
    const unsigned char *p=(const unsigned char *)data,*pcm=NULL; size_t pos=12,len,pcm_size=0;
    unsigned channels=0,bits_per_sample=0,rate=0,rate_code=0,align=0,id; int fmt=0,have_data=0;
    Buffer b={0},body={0};
    if(!available(c,name,e,cap)) return 0;
    if(!p || n<12 || n>16u*1024u*1024u || memcmp(p,"RIFF",4) || memcmp(p+8,"WAVE",4) || swl_u32(p+4)!=n-8) goto bad;
    while(pos<n) {
        if(n-pos<8) goto bad;
        len=swl_u32(p+pos+4); pos+=8;
        if(len>n-pos) goto bad;
        if(!memcmp(p+pos-8,"fmt ",4)) {
            if(fmt || len<16 || p[pos]!=1 || p[pos+1]) goto bad;
            channels=(unsigned)p[pos+2] | (unsigned)p[pos+3]<<8; rate=swl_u32(p+pos+4);
            align=(unsigned)p[pos+12] | (unsigned)p[pos+13]<<8;
            bits_per_sample=(unsigned)p[pos+14] | (unsigned)p[pos+15]<<8;
            if((channels!=1 && channels!=2) || (bits_per_sample!=8 && bits_per_sample!=16) || align!=channels*(bits_per_sample/8) || swl_u32(p+pos+8)!=rate*align) goto bad;
            fmt=1;
        } else if(!memcmp(p+pos-8,"data",4)) {
            if(have_data) goto bad;
            pcm=p+pos; pcm_size=len; have_data=1;
        }
        pos+=len;
        if(len&1) { if(pos==n) goto bad; ++pos; }
    }
    if(!fmt || !have_data || !pcm_size || pcm_size%align) goto bad;
    if(rate==5512) rate_code=0; else if(rate==11025) rate_code=1; else if(rate==22050) rate_code=2; else if(rate==44100) rate_code=3; else goto bad;
    id=(unsigned)c->asset_count*2+1; swl_word(&body,id);
    swl_byte(&body,0x30 | (rate_code<<2) | (bits_per_sample==16 ? 2 : 0) | (channels==2 ? 1 : 0));
    swl_dword(&body,(uint32_t)(pcm_size/align)); swl_bytes(&body,pcm,pcm_size);
    if(body.failed) b.failed=1; else swl_tag(&b,14,body.data,body.size);
    free(body.data); return save(c,name,&b,id,0,e,cap);
bad: return swl_fail(e,cap,"expected PCM WAV mono/stereo 8/16-bit at SWF sample rate");
}
static Asset *find(SwlContext *c, const char *name)
{
    size_t i;
    if(!c || !name) return NULL;
    for(i=0;i<c->asset_count;++i) if(!strcmp(c->assets[i].item.name,name)) return &c->assets[i];
    return NULL;
}
int swl_place(SwlContext *c, const char *name, unsigned x, unsigned y, unsigned depth, char *e, size_t cap)
{
    Asset *asset=find(c,name); Event event; size_t i;
    if(!asset || !asset->displayable || x>3276 || y>3276 || !depth || depth>65535 || c->event_count==SWL_ITEMS)
        return swl_fail(e,cap,"invalid placement/unknown display asset");
    for(i=0;i<c->event_count;++i) if(c->events[i].tag==26 && c->events[i].depth==depth) return swl_fail(e,cap,"duplicate display depth");
    event.tag=26; event.id=asset->id; event.x=x; event.y=y; event.depth=depth;
    c->events[c->event_count++]=event; return 1;
}
int swl_play(SwlContext *c, const char *name, char *e, size_t cap)
{
    Asset *asset=find(c,name); Event event={15,0,0,0,0};
    if(!asset || asset->displayable || c->event_count==SWL_ITEMS) return swl_fail(e,cap,"unknown sound/event limit");
    event.id=asset->id; c->events[c->event_count++]=event; return 1;
}
void swl_resources(SwlContext *c, Buffer *out)
{
    Buffer exports={0}; size_t i;
    if(c->asset_count) swl_word(&exports,(unsigned)c->asset_count);
    for(i=0;i<c->asset_count;++i) {
        Asset *asset=&c->assets[i];
        swl_bytes(out,asset->item.data,asset->item.size);
        swl_word(&exports,asset->id); swl_bytes(&exports,asset->item.name,strlen(asset->item.name)+1);
    }
    if(exports.failed) out->failed=1; else if(c->asset_count) swl_tag(out,56,exports.data,exports.size);
    free(exports.data);
    for(i=0;i<c->event_count;++i) {
        Event *event=&c->events[i]; Buffer body={0}; Bits stream;
        if(event->tag==15) { swl_word(&body,event->id); swl_byte(&body,0); }
        else {
            swl_byte(&body,6); swl_word(&body,event->depth); swl_word(&body,event->id);
            stream.buffer=&body; stream.used=0;
            bits(&stream,0,1); bits(&stream,0,1); bits(&stream,17,5);
            bits(&stream,event->x*20,17); bits(&stream,event->y*20,17);
        }
        if(body.failed) out->failed=1; else swl_tag(out,event->tag,body.data,body.size);
        free(body.data);
    }
}
