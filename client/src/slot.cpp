#include <slot.h>
#include <ui.h>
Slot::Slot(const vec2& p):pos(p){}
bool Slot::is_empty(){return value().empty();}
int Slot::get_item(){return value().empty()?-1:value().id;}
void Slot::set_item(int id,int count){value()={id,count};Items::normalize(value());}
bool Slot::is_point_over(vec2 p){return pos.x-3<=p.x && p.x<pos.x+51 && pos.y-3<=p.y && p.y<pos.y+51;}
void Slot::draw(){Ui::item(value(),pos.x+win_size.x/2.f,win_size.y/2.f-pos.y-48);}
