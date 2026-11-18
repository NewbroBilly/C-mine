#include <language.h>
#include <items.h>
#include <creative_tabs.h>
#include <iostream>
#include <cstdlib>
#include <algorithm>
void check(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
int main(){
    Language::russian=true;
    check(std::string(Language::tr("Singleplayer"))=="Одиночная игра","Russian menu");
    check(std::string(Items::name(Items::DIAMOND_PICKAXE))=="Алмазная кирка","Russian item name");
    check(std::string(CreativeTabs::name(7))=="Инструменты","Russian categories");
    for(int id:CreativeTabs::entries(5))check(std::string(Items::name(id))!=Items::english_name(id),"every supported item has a Russian name");
    auto matches=CreativeTabs::entries(5,"АЛМАЗНАЯ КИРКА");
    check(matches.size()==1 && matches[0]==Items::DIAMOND_PICKAXE,"case insensitive Russian search");
    matches=CreativeTabs::entries(5,"Diamond Pickaxe");
    check(matches.size()==1 && matches[0]==Items::DIAMOND_PICKAXE,"English aliases in Russian locale");
    check(Language::lower("ЁЖИК ёжик")=="ёжик ёжик","Cyrillic case folding including Yo");
    std::string name;Language::append(name,"Мой мир",32);Language::pop(name);
    check(name=="Мой ми" && Language::length(name)==6,"backspace removes a codepoint");
    Language::append(name,"р и ещё",7);check(name=="Мой мир","input limit counts characters");
    check(Language::tail("Новый мир",3)=="мир","text clipping preserves UTF-8");
    check(Language::glyph(0x410)==128 && Language::glyph(0x451)==193 && Language::glyph(0x1f600)=='?',"Cyrillic atlas and unsupported fallback");
    for(auto bad:{std::string("\xc0\xaf"),std::string("\xed\xa0\x80"),std::string("\xf4\x90\x80\x80"),std::string("\xd0")}){
        size_t i=0;check(Language::next(bad,i)=='?' && i>0 && i<=bad.size(),"reject malformed UTF-8 safely");
    }
    Language::russian=false;check(std::string(Items::name(Items::DIAMOND_PICKAXE))=="Diamond Pickaxe","English language switch");
    Language::russian=true;check(std::string(Language::tr("future key"))=="future key","missing translation fallback");
    std::cout<<"Russian/English UI, complete item names, bilingual search, UTF-8 input and malformed bytes passed\n";
}
