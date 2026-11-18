#pragma once
#include <glm.hpp>
#include <functional>

namespace Movement {
struct State {
    glm::dvec3 position{0};
    glm::dvec3 velocity{0};
    bool grounded = false;
};
using SolidQuery = std::function<bool(int, int, int)>;
bool overlaps(const glm::dvec3& position, const SolidQuery& solid);
void tick(State& state, const glm::dvec2& desired_velocity, bool jump,
          double dt, const SolidQuery& solid);
}
