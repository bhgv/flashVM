/* Raw historical IDs independently protect stored Transform callbacks from schema renumbering. */
#include "../src/standard/transform_state.hpp"
#include <cassert>
int main()
{
 const unsigned long ids[]={1400,1401,1410,1411,1412,1413,1414,1420,1421};
 for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);++i){
  FVM_NativeFunction callback=fvm_transform_callback_from_id(ids[i]);
  assert(callback && fvm_transform_callback_id(callback)==ids[i]);
  for(unsigned j=0;j<i;++j)assert(callback!=fvm_transform_callback_from_id(ids[j]));
 }
 const unsigned long invalid[]={0,1399,1402,1409,1415,1419,1422,~0UL};
 for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i)
  assert(!fvm_transform_callback_from_id(invalid[i]));
 assert(fvm_transform_callback_id(0)==0);
 return 0;
}
