#include <movement.h>
#include <algorithm>
#include <cmath>

namespace Movement {
bool overlaps(const glm::dvec3& p, const SolidQuery& solid) {
    constexpr double epsilon = 1e-7;
    for (int x = (int)std::floor(p.x - .3 + epsilon); x <= (int)std::floor(p.x + .3 - epsilon); ++x)
        for (int y = (int)std::floor(p.y + epsilon); y <= (int)std::floor(p.y + 1.8 - epsilon); ++y)
            for (int z = (int)std::floor(p.z - .3 + epsilon); z <= (int)std::floor(p.z + .3 - epsilon); ++z)
                if (solid(x, y, z)) return true;
    return false;
}

static bool move_axis(State& state, int axis, double distance, const SolidQuery& solid) {
    // Small swept intervals prevent crossing a block even at terminal velocity.
    const int steps = std::max(1, (int)std::ceil(std::abs(distance) / .2));
    const double step = distance / steps;
    for (int i = 0; i < steps; ++i) {
        glm::dvec3 target = state.position;
        target[axis] += step;
        if (overlaps(target, solid)) {
            double clear = 0, blocked = 1;
            for (int iteration = 0; iteration < 24; ++iteration) {
                const double fraction = (clear + blocked) * .5;
                target = state.position;
                target[axis] += step * fraction;
                if (overlaps(target, solid)) blocked = fraction;
                else clear = fraction;
            }
            state.position[axis] += step * clear;
            state.velocity[axis] = 0;
            return true;
        }
        state.position = target;
    }
    return false;
}

void tick(State& state, const glm::dvec2& target, bool jump, double dt, const SolidQuery& solid) {
    const double response = 1 - std::exp(-(state.grounded ? 18.0 : 7.0) * dt);
    state.velocity.x += (target.x - state.velocity.x) * response;
    state.velocity.z += (target.y - state.velocity.z) * response;
    if (jump && state.grounded) { state.velocity.y = 8.4; state.grounded = false; }
    state.velocity.y = std::max(-52.0, state.velocity.y - 28.0 * dt);
    move_axis(state, 0, state.velocity.x * dt, solid);
    move_axis(state, 2, state.velocity.z * dt, solid);
    const bool descending = state.velocity.y < 0;
    state.grounded = move_axis(state, 1, state.velocity.y * dt, solid) && descending;
    if (!state.grounded && state.velocity.y <= 0)
        state.grounded = overlaps(state.position - glm::dvec3(0, .002, 0), solid);
}
}
