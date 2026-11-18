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

void tick(State& state, const glm::dvec2& input, bool jump, bool sprint,
          bool sneak, const SolidQuery& solid, double slipperiness) {
    glm::dvec3 motion=state.velocity*.05;
    for(int axis=0;axis<3;++axis) if(std::abs(motion[axis])<.003)motion[axis]=0;
    if(state.jump_cooldown>0)--state.jump_cooldown;
    if(!jump)state.jump_cooldown=0;
    if(jump && state.grounded && state.jump_cooldown==0){
        motion.y=double(.42f);
        if(sprint && glm::length(input)>0){auto boost=glm::normalize(input)*.2;motion.x+=boost.x;motion.z+=boost.y;}
        state.jump_cooldown=10;
    }
    const bool was_grounded=state.grounded;
    double friction=was_grounded?slipperiness*double(.91f):double(.91f);
    double acceleration=was_grounded?(.1*(sprint?1.3:1.)*.16277136/(friction*friction*friction)):(sprint?.026:.02);
    glm::dvec2 control=input*(sneak?.3:1.);
    control*=.98;
    double magnitude=glm::length(control);
    if(magnitude>=1e-4){control/=std::max(1.,magnitude);motion.x+=control.x*acceleration;motion.z+=control.y*acceleration;}
    glm::dvec3 requested=motion;
    if(sneak && was_grounded){
        auto reduce=[](double v){return std::abs(v)<.05?0.:v-std::copysign(.05,v);};
        while(motion.x!=0 && !overlaps(state.position+glm::dvec3(motion.x,-1,0),solid))motion.x=reduce(motion.x);
        while(motion.z!=0 && !overlaps(state.position+glm::dvec3(0,-1,motion.z),solid))motion.z=reduce(motion.z);
        while(motion.x!=0 && motion.z!=0 && !overlaps(state.position+glm::dvec3(motion.x,-1,motion.z),solid)){motion.x=reduce(motion.x);motion.z=reduce(motion.z);}
    }
    const glm::dvec3 start=state.position;
    state.velocity=motion;
    bool descending=motion.y<0;
    bool y_hit=move_axis(state,1,motion.y,solid);
    bool x_hit=move_axis(state,0,motion.x,solid);
    bool z_hit=move_axis(state,2,motion.z,solid);
    if((x_hit || z_hit) && (was_grounded || (y_hit && descending))){
        State step=state;step.position=start;step.velocity=motion;
        move_axis(step,1,.6,solid);
        move_axis(step,0,motion.x,solid);move_axis(step,2,motion.z,solid);
        const double climbed=step.position.y-start.y;
        bool landed=move_axis(step,1,-climbed,solid);
        double normal=glm::length(glm::dvec2(state.position.x-start.x,state.position.z-start.z));
        double alternate=glm::length(glm::dvec2(step.position.x-start.x,step.position.z-start.z));
        if(alternate>normal+1e-7){state=step;state.velocity.x=motion.x;state.velocity.z=motion.z;y_hit=landed;descending=true;}
    }
    state.grounded=y_hit && descending;
    if(!state.grounded && requested.y<=0)state.grounded=overlaps(state.position-glm::dvec3(0,.002,0),solid);
    state.velocity.y=(state.velocity.y-.08)*double(.98f);
    state.velocity.x*=friction;state.velocity.z*=friction;
    state.velocity*=20.;
}
}
