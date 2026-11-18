#pragma once
#include <glm.hpp>
#include <functional>

namespace Movement {
struct State {
    glm::dvec3 position{0};
    glm::dvec3 velocity{0};
    bool grounded = false;
    int jump_cooldown = 0;
};
using SolidQuery = std::function<bool(int, int, int)>;
bool overlaps(const glm::dvec3& position, const SolidQuery& solid);
// One Java 1.12.2 land/air tick (20 Hz). Velocity is exposed in blocks/second.
void tick(State& state, const glm::dvec2& input, bool jump, bool sprint,
          bool sneak, const SolidQuery& solid, double slipperiness = .6);
}
