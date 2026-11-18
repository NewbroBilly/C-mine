#pragma once
namespace Menu {
void init();
void destroy();
// Returns a button index or -1. Input and drawing are independent.
int update(bool paused);
void draw(bool paused);
void reset();
void text_input(const char* text);
void wheel(int amount);
}
