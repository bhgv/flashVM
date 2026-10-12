#define main vector_fixture_main
#include "vector_paths.c"
#undef main
static void hit(FVM_Path *p,double x,double y,double width,int rule,int expected){int result=-1;assert(fvm_path_hit_test(p,x,y,width,rule,&result)==FVM_OK);assert(result==expected);}
int main(void){FVM_Path *p=0;int result;assert(fvm_path_create(&p)==FVM_OK);hit(p,0,0,0,0,0);add(p,FVM_PATH_MOVE,0,0,0,0,0,0);hit(p,0,0,0,0,0);rectangle(p,0,10);rectangle(p,3,7);hit(p,1,1,0,1,1);hit(p,5,5,0,1,0);hit(p,5,5,0,0,1);hit(p,11,5,0,0,0);hit(p,0,5,0,1,1);hit(p,10.5,5,2,0,1);hit(p,12,5,2,0,0);fvm_path_destroy(p);
assert(fvm_path_create(&p)==FVM_OK);add(p,FVM_PATH_MOVE,0,0,0,0,0,0);add(p,FVM_PATH_QUADRATIC,10,20,20,0,0,0);hit(p,10,5,0,0,1);hit(p,10,11,0,0,0);hit(p,10,10,1,0,1);hit(p,10,0,1,0,0);fvm_path_destroy(p);
assert(fvm_path_create(&p)==FVM_OK);add(p,FVM_PATH_MOVE,0,0,0,0,0,0);add(p,FVM_PATH_CUBIC,0,10,10,10,10,0);hit(p,5,5,0,0,1);hit(p,5,8,0,0,0);hit(p,5,7.5,1,0,1);fvm_path_destroy(p);
assert(fvm_path_create(&p)==FVM_OK);add(p,FVM_PATH_MOVE,1,1,0,0,0,0);add(p,FVM_PATH_LINE,9,1,0,0,0,0);add(p,FVM_PATH_LINE,1,9,0,0,0,0);hit(p,2,2,0,0,1);hit(p,8,8,0,0,0);hit(p,0.5,1,2,0,1);assert(fvm_path_hit_test(p,HUGE_VAL,0,0,0,&result)==FVM_INVALID_ARGUMENT);assert(fvm_path_hit_test(p,0,0,-1,0,&result)==FVM_INVALID_ARGUMENT);assert(fvm_path_hit_test(p,0,0,0,2,&result)==FVM_INVALID_ARGUMENT);assert(fvm_path_hit_test(p,0,0,0,0,0)==FVM_INVALID_ARGUMENT);fvm_path_destroy(p);return 0;}
