/* Shared native Array and TextField owners for custom tab-stop tests. */
#ifndef FVM_TEST_TEXTFIELD_TABS_FIXTURE_HPP
#define FVM_TEST_TEXTFIELD_TABS_FIXTURE_HPP
#define FVM_INSETS_FIXTURE_ONLY
#include "textfield_insets_fixture.hpp"
#include "flashvm/table.h"
#ifdef FVM_HAS_FONTS
static FVM_Object *tabs_array(FVM_Context *c,const double *stops,unsigned count)
{FVM_Object *a=0;assert(fvm_construct(c,"Array",0,0,&a)==FVM_OK && fvm_gc_pin(c,a)==FVM_OK);for(unsigned i=0;i<count;++i){FVM_Value v;fvm_value_number(&v,stops[i]);assert(fvm_table_set_index(c,a,i,&v)==FVM_OK);}return a;}
static FVM_Object *tabs_format(FVM_Context *c,const double *stops,unsigned count)
{FVM_Object *f=inset_format(c);inset_number(c,f,"leftMargin",0);inset_number(c,f,"rightMargin",0);inset_number(c,f,"blockIndent",0);FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=tabs_array(c,stops,count);assert(fvm_object_set(c,f,"tabStops",&v)==FVM_OK);return f;}
#endif
#endif
