#include "flashvm/stack.hpp"
#include <cassert>
int main() {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *ctx=0; FVM_Value value,out;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&ctx)==FVM_OK);
    { flashvm::Stack stack(ctx); assert(stack.valid());
      fvm_value_number(&value,7);
      assert(stack.push(value)==FVM_OK);
      assert(stack.get(-1,out)==FVM_OK && out.number==7);
      assert(stack.pop()==FVM_OK && stack.size()==0);
    }
    fvm_context_destroy(ctx); fvm_runtime_destroy(rt);return 0;
}
