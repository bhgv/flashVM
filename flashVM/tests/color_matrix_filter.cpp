#define FVM_HIT_FIXTURE_ONLY
#include "bitmap_hit_test.cpp"
#include "../src/standard/filter_state.hpp"
static inline FVM_Object *matrix_array(FVM_Context *c,const double *values)
{
    FVM_Object *a=palette(c);for(unsigned i=0;i<20;++i)palette_entry(c,a,i,values[i]);return a;
}
static inline void identity_matrix(double *values)
{
    for(unsigned i=0;i<20;++i)values[i]=(i==0 || i==6 || i==12 || i==18)?1:0;
}
static inline FVM_Object *filter(FVM_Context *c,FVM_Object *array=0,const char *name="flash.filters.ColorMatrixFilter")
{
    FVM_Object *o=0;FVM_Value a;if(array)a=ref(array);assert(fvm_construct(c,name,array?1:0,array?&a:0,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);return o;
}
static inline void filter_args(FVM_Value *a,FVM_Object *source,FVM_Object *rect,FVM_Object *point,FVM_Object *effect)
{
    a[0]=ref(source);a[1]=ref(rect);a[2]=ref(point);a[3]=ref(effect);
}
static inline FVM_Object *matrix_copy(FVM_Context *c,FVM_Object *effect)
{
    FVM_Value out;assert(fvm_object_get(c,effect,"matrix",&out)==FVM_OK);assert(out.type==FVM_VALUE_OBJECT);assert(out.object->resource_kind==28);assert(fvm_gc_pin(c,out.object)==FVM_OK);return out.object;
}
static inline double matrix_item(FVM_Context *c,FVM_Object *a,unsigned index)
{
    FVM_Value value;assert(fvm_table_get_index(c,a,index,&value)==FVM_OK);assert(value.type==FVM_VALUE_NUMBER);return value.number;
}
static inline void filter_packet(FVM_Context *c,FVM_Object *packet)
{
    FVM_Value image,rect,point,effect;assert(fvm_object_get(c,packet,"image",&image)==FVM_OK);assert(fvm_object_get(c,packet,"rect",&rect)==FVM_OK);assert(fvm_object_get(c,packet,"point",&point)==FVM_OK);assert(fvm_object_get(c,packet,"filter",&effect)==FVM_OK);
    assert(fvm_gc_pin(c,packet)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value clone=call(c,effect.object,"clone");assert(fvm_gc_pin(c,clone.object)==FVM_OK);assert(clone.object!=effect.object);FVM_Object *copy=matrix_copy(c,clone.object);near(matrix_item(c,copy,0),0);near(matrix_item(c,copy,2),1);
    FVM_Value a[4];filter_args(a,image.object,rect.object,point.object,clone.object);call(c,image.object,"applyFilter",4,a);assert(read(c,image.object,0,0)==0x803264c8UL);
    FVM_Value bounds_args[]={rect,effect};FVM_Value bounds=call(c,image.object,"generateFilterRect",2,bounds_args);assert(bounds.object!=rect.object);near(get(c,bounds.object,"width"),2);
}
#ifndef FVM_COLOR_FILTER_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    double values[20];identity_matrix(values);FVM_Object *array=matrix_array(c,values),*effect=filter(c,array),*a=matrix_copy(c,effect),*b=matrix_copy(c,effect);assert(a!=b);palette_entry(c,array,0,2);near(matrix_item(c,a,0),1);palette_entry(c,a,0,3);near(matrix_item(c,matrix_copy(c,effect),0),1);
    FVM_Value assignment=ref(a);assert(fvm_object_set(c,effect,"matrix",&assignment)==FVM_OK);near(matrix_item(c,matrix_copy(c,effect),0),3);FVM_Value clone=call(c,effect,"clone");assert(fvm_gc_pin(c,clone.object)==FVM_OK);assert(clone.object!=effect);palette_entry(c,a,0,4);assignment=ref(a);assert(fvm_object_set(c,clone.object,"matrix",&assignment)==FVM_OK);near(matrix_item(c,matrix_copy(c,effect),0),3);
    FVM_Object *base=filter(c,0,"flash.filters.BitmapFilter");FVM_Value base_clone=call(c,base,"clone");assert(base_clone.object!=base);FVM_Value base_class,color_class,space,flash;assert(fvm_get_global(c,"flash",&flash)==FVM_OK);assert(fvm_object_get(c,flash.object,"filters",&space)==FVM_OK);assert(fvm_object_get(c,space.object,"BitmapFilter",&base_class)==FVM_OK);assert(fvm_object_get(c,space.object,"ColorMatrixFilter",&color_class)==FVM_OK);int result=0;FVM_Value value=ref(effect);assert(fvm_value_instance_of(c,&value,&base_class,&result)==FVM_OK && result);assert(fvm_value_instance_of(c,&value,&color_class,&result)==FVM_OK && result);
    identity_matrix(values);values[0]=0;values[2]=1;values[10]=1;values[12]=0;array=matrix_array(c,values);effect=filter(c,array);FVM_Object *image=bitmap(c,2,1,true,0x80c86432UL),*dest=bitmap(c,2,1,true,0);double box[]={0,0,2,1},xy[]={0,0};FVM_Object *rect=region_object(c,"flash.geom.Rectangle",4,box),*point=hit_point(c,0,0);FVM_Value args[4];filter_args(args,image,rect,point,effect);call(c,dest,"applyFilter",4,args);assert(read(c,dest,0,0)==0x803264c8UL);
    identity_matrix(values);values[0]=0.5;values[4]=10.5;values[6]=0;values[9]=-10;values[12]=10;values[18]=0;values[19]=255;FVM_Object *clamp=filter(c,matrix_array(c,values));filter_args(args,image,rect,point,clamp);call(c,dest,"applyFilter",4,args);assert(read(c,dest,0,0)==0xff6f00ffUL);
    identity_matrix(values);values[18]=0;FVM_Object *clear=filter(c,matrix_array(c,values));filter_args(args,image,rect,point,clear);call(c,dest,"applyFilter",4,args);assert(read(c,dest,0,0)==0);FVM_Object *opaque=bitmap(c,2,1,false,0);call(c,opaque,"applyFilter",4,args);assert(read(c,opaque,0,0)==0xffc86432UL);
    FVM_Object *alias=bitmap(c,3,1,true,0xffff0000UL);write(c,alias,1,0,0xff00ff00UL);write(c,alias,2,0,0xff0000ffUL);FVM_Object *shift=hit_point(c,1,0);filter_args(args,alias,rect,shift,effect);call(c,alias,"applyFilter",4,args);assert(read(c,alias,0,0)==0xffff0000UL && read(c,alias,1,0)==0xff0000ffUL && read(c,alias,2,0)==0xff00ff00UL);
    FVM_Value bounds_args[]={ref(rect),ref(effect)};FVM_Value bounds=call(c,dest,"generateFilterRect",2,bounds_args);assert(bounds.object!=rect && bounds.object->resource_kind==FVM_GEOMETRY_RECTANGLE);near(get(c,bounds.object,"width"),2);set(c,bounds.object,"width",9);near(get(c,rect,"width"),2);
    FVM_Object *packet=0;assert(fvm_object_create(c,&packet)==FVM_OK);assert(fvm_gc_pin(c,packet)==FVM_OK);value=ref(image);assert(fvm_object_set(c,packet,"image",&value)==FVM_OK);value=ref(rect);assert(fvm_object_set(c,packet,"rect",&value)==FVM_OK);value=ref(point);assert(fvm_object_set(c,packet,"point",&value)==FVM_OK);value=ref(effect);assert(fvm_object_set(c,packet,"filter",&value)==FVM_OK);value=ref(packet);FVM_Channel *ch=0;FVM_Value copy;assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);filter_packet(other,copy.object);fvm_channel_destroy(ch);
    FVM_Result opened=fvm_channel_open_file("filter-test.bin","w",&ch);if(opened==FVM_OK){assert(fvm_channel_send_value(c,ch,&value,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("filter-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK);fvm_channel_destroy(ch);filter_packet(other,copy.object);assert(std::remove("filter-test.bin")==0);}else assert(opened==FVM_UNSUPPORTED);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&copy,0)==FVM_OK);filter_packet(other,copy.object);fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    unsigned long previous=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=1;filter_args(args,image,rect,point,effect);FVM_Value out;assert(fvm_object_call(c,dest,"applyFilter",4,args,&out)==FVM_OUT_OF_MEMORY);rt->config.memory_limit_bytes=previous;assert(read(c,dest,0,0)==0);
    args[3]=ref(base);assert(fvm_object_call(c,dest,"applyFilter",4,args,&out)==FVM_UNSUPPORTED);FVM_Object *invalid=palette(c);assignment=ref(invalid);assert(fvm_object_set(c,effect,"matrix",&assignment)==FVM_INVALID_ARGUMENT);near(matrix_item(c,matrix_copy(c,effect),2),1);
    identity_matrix(values);values[0]=std::numeric_limits<double>::infinity();invalid=matrix_array(c,values);assignment=ref(invalid);assert(fvm_object_set(c,effect,"matrix",&assignment)==FVM_INVALID_ARGUMENT);near(matrix_item(c,matrix_copy(c,effect),2),1);
    FVM_Object *disposed=bitmap(c,2,1,true,0xff000000UL),*callback=0;assert(fvm_object_create(c,&callback)==FVM_OK);assert(fvm_gc_pin(c,callback)==FVM_OK);set(c,callback,"y",0);set(c,callback,"width",2);set(c,callback,"height",1);assert(fvm_object_define_accessor_rooted(c,callback,"x",hit_dispose,0,disposed,disposed)==FVM_OK);filter_args(args,image,callback,point,effect);assert(call(c,disposed,"applyFilter",4,args).number==-1);
    assert(call(c,disposed,"generateFilterRect",2,bounds_args).number==-1);
    double private_values[20];assert(fvm_filter_matrix(c,effect,private_values));FVM_Object *private_state=effect->resource_result.object;private_state->properties["0"].number=1000001;unsigned char *bytes=0;size_t size=0;value=ref(effect);assert(fvm_value_serialize(c,&value,&bytes,&size)==FVM_UNSUPPORTED);private_state->properties["0"].number=private_values[0];
    private_state->properties["0"].number=12345.75;assert(fvm_value_serialize(c,&value,&bytes,&size)==FVM_OK);
    double marker=12345.75,invalid_coefficient=1000001.0,one=1;unsigned char host_marker[8],host_invalid[8],one_bytes[8],pattern[8],changed[8];std::memcpy(host_marker,&marker,8);std::memcpy(host_invalid,&invalid_coefficient,8);std::memcpy(one_bytes,&one,8);bool little=one_bytes[0]==0;
    for(unsigned i=0;i<8;++i){pattern[i]=host_marker[little?7-i:i];changed[i]=host_invalid[little?7-i:i];}unsigned matches=0;for(size_t i=0;i+8<=size;++i)if(std::memcmp(bytes+i,pattern,8)==0){std::memcpy(bytes+i,changed,8);++matches;}assert(matches==1);
    assert(fvm_value_deserialize(other,bytes,size,&out)==FVM_INVALID_ARGUMENT);fvm_serialized_free(bytes);private_state->properties["0"].number=private_values[0];
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
