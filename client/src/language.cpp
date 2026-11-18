#include <language.h>
#include <unordered_map>
namespace Language {
const char* tr(const char* english){
    static const std::unordered_map<std::string,const char*> ru={
#include <russian.def>
    };
    if(!russian)return english;auto i=ru.find(english);return i==ru.end()?english:i->second;
}
uint32_t next(const std::string& s,size_t& i){
    if(i>=s.size())return 0;uint32_t c=(unsigned char)s[i++];if(c<128)return c;
    int n=c>=0xc2 && c<=0xdf?1:c>=0xe0 && c<=0xef?2:c>=0xf0 && c<=0xf4?3:0;
    if(!n)return '?';uint32_t cp=c&((1u<<(6-n))-1);size_t end=i+n;
    if(end>s.size())return '?';
    for(size_t j=i;j<end;++j){uint32_t b=(unsigned char)s[j];if((b&0xc0)!=0x80)return '?';cp=(cp<<6)|(b&63);}
    i=end;if(cp<(n==1?128u:n==2?2048u:65536u) || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff))return '?';return cp;
}
static void encode(std::string& out,uint32_t cp){
    if(cp<128)out+=char(cp);
    else if(cp<2048){out+=char(0xc0|(cp>>6));out+=char(0x80|(cp&63));}
    else if(cp<65536){out+=char(0xe0|(cp>>12));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
    else{out+=char(0xf0|(cp>>18));out+=char(0x80|((cp>>12)&63));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
}
std::string lower(const std::string& s){std::string out;for(size_t i=0;i<s.size();){auto cp=next(s,i);if(cp>='A' && cp<='Z')cp+=32;else if(cp>=0x410 && cp<=0x42f)cp+=32;else if(cp==0x401)cp=0x451;encode(out,cp);}return out;}
size_t length(const std::string& s){size_t n=0;for(size_t i=0;i<s.size();++n)next(s,i);return n;}
void pop(std::string& s){size_t last=0;for(size_t i=0;i<s.size();){last=i;next(s,i);}s.resize(last);}
void append(std::string& s,const std::string& input,size_t max){size_t count=length(s);for(size_t i=0;i<input.size() && count<max;){auto cp=next(input,i);if(cp>=32 && cp!=127){encode(s,cp);++count;}}}
std::string tail(const std::string& s,size_t count){auto n=length(s);size_t i=0;while(n>count){next(s,i);--n;}return s.substr(i);}
int glyph(uint32_t cp){if(cp<128)return int(cp);if(cp>=0x410 && cp<=0x44f)return 128+int(cp-0x410);if(cp==0x401)return 192;if(cp==0x451)return 193;return '?';}
}
