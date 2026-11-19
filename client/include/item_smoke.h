#pragma once
#include <functional>
// Requires the native GL smoke world's loaded chunks; never runs in normal play.
bool item_smoke(const std::function<void(const char*)>& save_frame);
