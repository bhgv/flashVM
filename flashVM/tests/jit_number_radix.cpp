/* Loaded Int32 radix output agrees across interpreter, explicit and automatic native CallMethod. */
#define FVM_LOADED_RADIX_FIXTURE_ONLY
#include "number_loaded_radix.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
/* Two 17-body groups keep automatic native checks below the existing 32-entry fallback bound. */
int main()
{
 if(!fvm_jit_available())return 0;FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(unsigned batch=0;batch<2;++batch){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);radix_loaded(c,version);
  const double inputs[]={42.75,-42.75,2147483647.5,-2147483648.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()};
  for(unsigned i=0;i<6;++i){FVM_Value value=object(radix_box(c,inputs[i]));assert(fvm_set_global(c,"numberInput",&value)==FVM_OK);
   for(unsigned j=batch*17;j<(batch+1)*17;++j){const LoadedRadixExpected &e=loaded_radix_expected[j];Program p;p.number(e.radix);p.number(1);p.variable("numberInput");p.text("toString");p.op(0x52);p.finish();value=class_run(c,p);
    const char *expected=i>=2?e.sentinel:i?e.negative:e.positive;assert(value.type==FVM_VALUE_STRING && fvm_value_string_size(&value)==std::strlen(expected) && !std::memcmp(value.string,expected,std::strlen(expected)));
   }
  }
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
