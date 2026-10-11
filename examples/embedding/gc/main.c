/* C89 example: explicitly root an object held by native code. */
#include "flashvm/gc.h"
#include <stdio.h>
int main(void) {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *vm=0;
    FVM_Object *object=0; unsigned long reclaimed=0;
    if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK) return 1;
    if(fvm_context_create(rt,&vm)!=FVM_OK) {fvm_runtime_destroy(rt);return 2;}
    if(fvm_object_create(vm,&object)!=FVM_OK || fvm_gc_pin(vm,object)!=FVM_OK) return 3;
    fvm_gc_collect(vm,&reclaimed); /* object survives */
    printf("First collection: %lu objects reclaimed\n",reclaimed);
    fvm_gc_unpin(vm,object); /* never use object after collection */
    fvm_gc_collect(vm,&reclaimed);
    printf("Second collection: %lu objects reclaimed\n",reclaimed);
    fvm_context_destroy(vm); fvm_runtime_destroy(rt);return 0;
}
