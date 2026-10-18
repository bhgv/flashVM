#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
static unsigned count;
static unsigned order[4];
static FVM_Result convert(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u){unsigned id=*(unsigned *)u;order[count++]=id;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_number(out,id==1?9:2);return FVM_OK;}
static FVM_Result fail(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *){return FVM_ERROR;}
static FVM_Value operation(FVM_Context *c,unsigned op,const FVM_Value &a,const FVM_Value &b){assert(fvm_set_global(c,"left",&a)==FVM_OK);assert(fvm_set_global(c,"right",&b)==FVM_OK);Bytes bytes;global(bytes,"left");if(op!=0x50 && op!=0x51)global(bytes,"right");bytes.push_back((unsigned char)op);return run(c,bytes);}
static FVM_Value number(double n){FVM_Value v;fvm_value_number(&v,n);return v;}
int main(){FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
struct Case{unsigned op;double a,b,result;};const Case cases[]={
{0x60,0xffffffffUL,0x80000001UL,-2147483647.0},{0x61,0x80000000UL,1,-2147483647.0},{0x62,0xffffffffUL,0x7fffffffUL,-2147483648.0},
{0x63,0x40000001UL,1,-2147483646.0},{0x63,0xffffffffUL,32,-1},{0x63,1,33,2},{0x63,1,-1,-2147483648.0},
{0x64,0x80000000UL,1,-1073741824.0},{0x64,0x80000000UL,31,-1},{0x64,0xffffffffUL,0,-1},
{0x65,0x80000000UL,1,1073741824.0},{0x65,0xffffffffUL,0,4294967295.0},{0x65,-1,31,1},
{0x60,4294967297.0,3,1},{0x60,-1.9,15,15},{0x3f,9,2,1},{0x3f,-9,2,-1},{0x3f,9,-2,1},{0x50,1.5,0,2.5},{0x51,1.5,0,0.5}};
for(size_t i=0;i<sizeof(cases)/sizeof(*cases);++i){FVM_Value out=operation(c,cases[i].op,number(cases[i].a),number(cases[i].b));assert(out.type==FVM_VALUE_NUMBER && out.number==cases[i].result);}
assert(operation(c,0x67,number(3),number(2)).boolean);assert(!operation(c,0x67,number(2),number(3)).boolean);assert(!operation(c,0x67,number(std::numeric_limits<double>::quiet_NaN()),number(3)).boolean);FVM_Value string_a,string_b;fvm_value_string(&string_a,"z");fvm_value_string(&string_b,"a");assert(operation(c,0x67,string_a,string_b).boolean);
FVM_Value text_value;fvm_value_string(&text_value,"4294967297");assert(operation(c,0x63,text_value,number(1)).number==2);
fvm_value_string(&text_value,"1.5");assert(operation(c,0x50,text_value,number(0)).number==2.5);
FVM_Value out=operation(c,0x3f,number(1),number(0));assert(out.number!=out.number);
const double inf=std::numeric_limits<double>::infinity(),nan=std::numeric_limits<double>::quiet_NaN();
out=operation(c,0x3f,number(inf),number(2));assert(out.number!=out.number);assert(operation(c,0x3f,number(-7),number(inf)).number==-7);
assert(operation(c,0x60,number(nan),number(3)).number==0);assert(operation(c,0x61,number(inf),number(3)).number==3);
out=operation(c,0x3f,number(-0.0),number(2));assert(out.number==0 && 1.0/out.number<0);
FVM_Object *left=0,*right=0;assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,left,"valueOf",convert,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,right,"valueOf",convert,ids+1,0)==FVM_OK);FVM_Value a,b;fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=left;b=a;b.object=right;count=0;assert(operation(c,0x3f,a,b).number==1);assert(count==2 && order[0]==2 && order[1]==1);count=0;assert(operation(c,0x67,a,b).boolean);assert(count==2 && order[0]==2 && order[1]==1);
/* Legacy comparison coerces numeric strings rather than comparing them lexically. */
fvm_value_string(&string_a,"10");fvm_value_string(&string_b,"2");
assert(!operation(c,0x0f,string_a,string_b).boolean);
assert(operation(c,0x0e,string_a,number(10)).boolean);
fvm_value_string(&string_a,"invalid");assert(!operation(c,0x0e,string_a,string_a).boolean);
assert(!operation(c,0x10,number(nan),number(1)).boolean);
assert(operation(c,0x11,number(nan),number(1)).boolean);
fvm_value_boolean(&string_a,1);assert(operation(c,0x10,string_a,number(1)).boolean);
count=0;assert(!operation(c,0x0e,a,b).boolean);assert(count==2 && order[0]==2 && order[1]==1);
count=0;assert(!operation(c,0x0f,a,b).boolean);assert(count==2 && order[0]==1 && order[1]==2);
count=0;assert(operation(c,0x10,a,b).boolean && count==0);
assert(fvm_object_define_method_rooted(c,left,"valueOf",fail,0,0)==FVM_OK);Bytes broken;global(broken,"left");broken.push_back(0x50);broken.push_back(0);assert(fvm_avm1_execute(c,broken.begin(),broken.size())==FVM_ERROR);assert(operation(c,0x50,number(1),number(0)).number==2);
for(unsigned op=0x0e;op<=0x0f;++op){assert(fvm_set_global(c,"left",&a)==FVM_OK);assert(fvm_set_global(c,"right",&b)==FVM_OK);Bytes bad;global(bad,"left");global(bad,"right");bad.push_back((unsigned char)op);bad.push_back(0);count=0;assert(fvm_avm1_execute(c,bad.begin(),bad.size())==FVM_ERROR);assert(count==(op==0x0e?1U:0U));assert(operation(c,op,number(1),number(1)).boolean==(op==0x0e));}
for(unsigned op=0x60;op<=0x65;++op){unsigned char underflow[]={(unsigned char)op,0};assert(fvm_avm1_execute(c,underflow,sizeof(underflow))==FVM_ERROR);}
fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;}
