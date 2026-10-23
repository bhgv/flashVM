#include "cubic_scale_fixture.hpp"
int main(){for(unsigned axis=0;axis<3;++axis)for(unsigned reverse=0;reverse<2;++reverse)for(int e=-400;e>=-1000;e-=100)cb_check(5,e,axis,reverse!=0);return 0;}
