/* Independent raw SWF fixtures for DefineSound metadata, PCM and submission tests. */
#ifndef FVM_TEST_SWF_SOUND_FIXTURE_HPP
#define FVM_TEST_SWF_SOUND_FIXTURE_HPP
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
namespace sound_fixture {
typedef fvm_detail::vector<unsigned char> Bytes;
inline void word(Bytes &b,unsigned n){b.push_back((unsigned char)n);b.push_back((unsigned char)(n>>8));}
inline void integer(Bytes &b,unsigned long n){for(unsigned i=0;i<4;++i){b.push_back((unsigned char)n);n>>=8;}}
inline void tag(Bytes &b,unsigned code,const Bytes &payload){if(payload.size()<63)word(b,(code<<6)|(unsigned)payload.size());else{word(b,(code<<6)|63);integer(b,payload.size());}for(size_t i=0;i<payload.size();++i)b.push_back(payload[i]);}
inline Bytes sound(unsigned id,unsigned flags,unsigned long frames,const unsigned char *data,size_t size){Bytes p;word(p,id);p.push_back((unsigned char)flags);integer(p,frames);for(size_t i=0;i<size;++i)p.push_back(data[i]);return p;}
inline Bytes movie(const Bytes &definitions,unsigned version=8){Bytes tags=definitions,empty;tag(tags,1,empty);tag(tags,0,empty);Bytes b;b.push_back('F');b.push_back('W');b.push_back('S');b.push_back((unsigned char)version);integer(b,tags.size()+14);b.push_back(8);b.push_back(0);word(b,12*256);word(b,1);for(size_t i=0;i<tags.size();++i)b.push_back(tags[i]);return b;}
}
#endif
