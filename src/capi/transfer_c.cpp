#include "../core/runtime.hpp"
#include "../core/channel_packet.h"
#include "flashvm/transfer.h"
#include <new>
#include <cstring>
namespace {
const size_t no_object=(size_t)-1;
struct Value {
    FVM_ValueType type;double number;int boolean;fvm_detail::string string;size_t object;
    Value():type(FVM_VALUE_UNDEFINED),number(0),boolean(0),object(no_object) {}
};
struct Object {
    fvm_detail::map<fvm_detail::string,Value> properties;
    fvm_detail::map<unsigned long,Value> indexed;
    size_t prototype,metatable;
    Object():prototype(no_object),metatable(no_object) {}
};
struct Packet {
    Value value;
    fvm_detail::vector<Object> objects;
};
static void release(void *p) {delete (Packet *)p;}
static FVM_Result identify(FVM_Context *c,FVM_Object *o,fvm_detail::vector<FVM_Object *> &seen,size_t *id) {
    if(!o) {*id=no_object;return FVM_OK;}
    if(o->owner!=c) return FVM_INVALID_ARGUMENT;
    for(size_t i=0;i<seen.size();++i)if(seen[i]==o){*id=i;return FVM_OK;}
    if(seen.size()>=4096) return FVM_UNSUPPORTED;
    *id=seen.size();seen.push_back(o);return FVM_OK;
}
static FVM_Result capture(FVM_Context *c,const FVM_Value &src,Value &dst,fvm_detail::vector<FVM_Object *> &seen) {
    if(src.type<FVM_VALUE_UNDEFINED || src.type>FVM_VALUE_OBJECT) return FVM_INVALID_ARGUMENT;
    dst.type=src.type;dst.number=src.number;dst.boolean=src.boolean;
    if(src.type==FVM_VALUE_STRING) {if(!src.string)return FVM_INVALID_ARGUMENT;dst.string=src.string;}
    if(src.type==FVM_VALUE_OBJECT) {if(!src.object)return FVM_INVALID_ARGUMENT;return identify(c,src.object,seen,&dst.object);}
    return FVM_OK;
}
static FVM_Result snapshot(FVM_Context *c,const FVM_Value &v,Packet *p) {
    fvm_detail::vector<FVM_Object *> seen;
    FVM_Result r=capture(c,v,p->value,seen);if(r!=FVM_OK)return r;
    for(size_t i=0;i<seen.size();++i) {
        FVM_Object *src=seen[i];Object dst;
        if(src->resource || src->callable.callback || !src->metamethods.empty() || !src->methods.empty()) return FVM_UNSUPPORTED;
        r=identify(c,src->prototype,seen,&dst.prototype);if(r!=FVM_OK)return r;
        r=identify(c,src->metatable,seen,&dst.metatable);if(r!=FVM_OK)return r;
        fvm_detail::map<fvm_detail::string,FVM_Value>::iterator named;
        for(named=src->properties.begin();named!=src->properties.end();++named) {
            Value value;r=capture(c,named->second,value,seen);if(r!=FVM_OK)return r;
            dst.properties[named->first]=value;
        }
        fvm_detail::map<unsigned long,FVM_Value>::iterator indexed;
        for(indexed=src->indexed.begin();indexed!=src->indexed.end();++indexed) {
            Value value;r=capture(c,indexed->second,value,seen);if(r!=FVM_OK)return r;
            dst.indexed[indexed->first]=value;
        }
        p->objects.push_back(dst);
    }
    return FVM_OK;
}
static FVM_Result restore_value(FVM_Context *c,const Value &src,FVM_Value *out,const fvm_detail::vector<FVM_Object *> &objects) {
    fvm_value_undefined(out);out->type=src.type;out->number=src.number;out->boolean=src.boolean;
    if(src.type==FVM_VALUE_OBJECT)out->object=objects[src.object];
    if(src.type==FVM_VALUE_STRING) {
        char *copy=new(std::nothrow) char[src.string.size()+1];if(!copy)return FVM_OUT_OF_MEMORY;
        std::memcpy(copy,src.string.c_str(),src.string.size()+1);
        try {c->strings.push_back(copy);}catch(...){delete [] copy;throw;}
        out->string=copy;
    }
    return FVM_OK;
}
static FVM_Result restore(FVM_Context *c,Packet *p,FVM_Value *out) {
    fvm_detail::vector<FVM_Object *> objects;
    for(size_t i=0;i<p->objects.size();++i) {
        FVM_Object *o=0;FVM_Result r;
        r=fvm_object_create(c,&o);
        if(r!=FVM_OK)return r;
        objects.push_back(o);
    }
    for(size_t i=0;i<objects.size();++i) {
        const Object &src=p->objects[i];FVM_Object *dst=objects[i];
        if(src.prototype!=no_object)dst->prototype=objects[src.prototype];
        if(src.metatable!=no_object)dst->metatable=objects[src.metatable];
        fvm_detail::map<fvm_detail::string,Value>::iterator named;
        for(named=src.properties.begin();named!=src.properties.end();++named) {
            FVM_Value v;FVM_Result r=restore_value(c,named->second,&v,objects);if(r!=FVM_OK)return r;
            dst->properties[named->first]=v;
        }
        fvm_detail::map<unsigned long,Value>::iterator indexed;
        for(indexed=src.indexed.begin();indexed!=src.indexed.end();++indexed) {
            FVM_Value v;FVM_Result r=restore_value(c,indexed->second,&v,objects);if(r!=FVM_OK)return r;
            dst->indexed[indexed->first]=v;
        }
    }
    return restore_value(c,p->value,out,objects);
}
}
extern "C" FVM_Result fvm_channel_send_value(FVM_Context *c,FVM_Channel *ch,const FVM_Value *v,int blocking) {
    if(!c || !ch || !v || c->destroying) return FVM_INVALID_ARGUMENT;
    if(v->type!=FVM_VALUE_OBJECT)return fvm_channel_send_primitive(ch,v,blocking);
    Packet *p=0;
    try {p=new Packet;}catch(...){return FVM_OUT_OF_MEMORY;}
    FVM_Result r;
    try {r=snapshot(c,*v,p);if(r==FVM_OK)r=fvm_channel_packet_send(ch,p,release,blocking);}
    catch(...) {r=FVM_OUT_OF_MEMORY;}
    if(r!=FVM_OK)delete p;
    return r;
}
extern "C" FVM_Result fvm_channel_receive_value(FVM_Context *c,FVM_Channel *ch,FVM_Value *out,int blocking) {
    if(!c || !ch || !out || c->destroying)return FVM_INVALID_ARGUMENT;
    void *packet=0;FVM_Value v;FVM_Result r=fvm_channel_receive_mixed(ch,&v,&packet,blocking);
    if(r!=FVM_OK)return r;
    fvm_value_undefined(out);
    if(!packet) {
        try {
            *out=v;
            if(v.type==FVM_VALUE_STRING) {
                size_t len=std::strlen(v.string);char *copy=new(std::nothrow) char[len+1];
                if(!copy)r=FVM_OUT_OF_MEMORY;
                else {
                    std::memcpy(copy,v.string,len+1);
                    try {c->strings.push_back(copy);}catch(...){delete [] copy;throw;}
                    out->string=copy;
                }
            }
        }catch(...) {r=FVM_OUT_OF_MEMORY;}
        fvm_channel_message_release(&v);
    } else {
        try {r=restore(c,(Packet *)packet,out);}catch(...) {r=FVM_OUT_OF_MEMORY;}
        release(packet);
    }
    if(r!=FVM_OK)fvm_value_undefined(out);
    return r;
}
