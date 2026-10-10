/* C89: create an AVM1 object and expose it to scripts through globals. */
#include "flashvm/value.h"
#include <stdio.h>
int main(void) {
 FVM_Config cfg;FVM_Runtime *r=0;FVM_Context *c=0;FVM_Object *o=0;FVM_Value v;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&r)!=FVM_OK || fvm_context_create(r,&c)!=FVM_OK) return 1;
 if(fvm_object_create(c,&o)!=FVM_OK) return 2;
 fvm_value_number(&v,123);
 if(fvm_object_set(c,o,"score",&v)!=FVM_OK) return 3;
 fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
 if(fvm_set_global(c,"gameState",&v)!=FVM_OK) return 4;
 if(fvm_object_get(c,o,"score",&v)!=FVM_OK || v.number!=123) return 5;
 printf("gameState.score = %.0f\n",v.number);
 fvm_context_destroy(c);fvm_runtime_destroy(r);return 0;
}
