#pragma once
#include <glm.hpp>
// All six faces share these bounds; shrinking only the face normal makes fins.
inline glm::vec3 torch_corner(glm::vec3 unit_corner) {
    return glm::vec3(7.f/16,0,7.f/16) + unit_corner*glm::vec3(2.f/16,10.f/16,2.f/16);
}
