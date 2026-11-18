#pragma once
#include <items.h>
#include <string>
namespace CreativeTabs {
constexpr int Count=12;
const char* name(int tab);
int icon(int tab);
std::vector<int> entries(int tab,const std::string& search="");
}
