#include "../src/details/string.hpp"
#include "../src/details/hash_table.hpp"
#include <cstring>
int main() {
    fvm_detail::string empty;
    fvm_detail::string a("hello"), b(a), c("hello", 5);
    if(!empty.empty() || !(a==b) || !(b==c) || a.size()!=5) return 1;
    b="world";
    if(a==b || std::strcmp(b.c_str(),"world")) return 2;
    const char bytes[3]={'a',0,'b'};
    fvm_detail::string binary(bytes,3);
    if(binary.size()!=3 || binary[2]!='b' || binary==fvm_detail::string("a"))return 3;
    fvm_detail::map<fvm_detail::string,int> m;
    m[a]=10; m[b]=20; m[binary]=30;
    if(m.size()!=3 || m.find("hello")->second!=10 || m.find(binary)->second!=30)return 4;
    fvm_detail::map<fvm_detail::string,int> clone(m);
    if(clone.find("world")->second!=20)return 5;
    return 0;
}
