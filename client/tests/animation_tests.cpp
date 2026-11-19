#include <model_animation.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace ModelAnimation;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
bool near(float a,float b){return std::abs(a-b)<1e-5f;}
int main(){try{
    Walk w;w.tick(.1,0);check(near(w.amount,.16f) && near(w.phase,.16f),"EntityLivingBase: movement*4, clamp, .4 smoothing");
    check(near(w.render_phase(0),0) && near(w.render_phase(1),.16f),"partial tick limb phase");
    check(near(w.render_amount(.5f),.08f),"partial tick limb amplitude");
    w.tick(0,0);check(near(w.amount,.096f),"stopping decays rather than snapping into a frozen wide stride");
    Walk fast;fast.tick(2,0);check(near(fast.amount,.4f),"displacement target is capped at one");
    auto p=biped(0,1,0,0,0);
    check(near(p.right_arm.x,-1) && near(p.left_arm.x,1) && near(p.right_leg.x,1.4f) && near(p.left_leg.x,-1.4f),"opposite biped limbs, original .6662/1.4 constants");
    check(near(p.right_arm.z,.1f) && near(p.left_arm.z,-.1f),"idle breathing uses entity age, not walk distance");
    p=biped(0,0,20,.4f,.5f);
    float twist=std::sin(std::sqrt(.5f)*pi*2)*.2f;
    check(near(p.body.y,twist) && near(p.right_arm.px,-std::cos(twist)*5) && near(p.left_arm.pz,-std::sin(twist)*5),"attack twists body and shoulder pivots");
    float expected=-std::sin((1-std::pow(.5f,4.f))*pi)*1.2f-std::sin(.5f*pi)*-(.4f-.7f)*.75f+std::sin(20*.067f)*.05f;
    check(near(p.right_arm.x,expected) && near(p.right_arm.z,-.4f+std::cos(20*.09f)*.05f+.05f),"quartic attack easing, head correction and roll");
    p=biped(0,0,0,0,0,true);check(near(p.body.x,.5f) && near(p.right_leg.py,9) && near(p.left_leg.pz,4) && near(p.head.py,1),"original crouch pivots");
    p=biped(0,0,0,0,0,false,true);check(near(p.right_arm.x,-pi/10),"held item arm pose");
    p=biped(0,0,0,0,0,false,false,true);check(near(p.right_arm.x,-.9424779f) && near(p.right_arm.y,-.5235988f),"shield blocking pose");
    p=biped(2,.8f,42,.3f,.4f,false,true,false,true);
    check(near(p.right_arm.x,-pi/2+.3f) && near(p.left_arm.x,p.right_arm.x) && near(p.right_arm.y,-.1f) && near(p.left_arm.y,.5f),"bow pose overrides arm pitch/yaw after idle and attack");
    auto z=zombie(0,0,0,0,false),raised=zombie(0,0,0,0,true);
    check(near(z.right_arm.x,-pi/2.25f) && near(raised.right_arm.x,-pi/1.5f),"ModelZombie idle/raised arms, not a fixed -pi/2 approximation");
    check(near(z.right_arm.y,-.1f) && near(z.left_arm.y,.1f),"zombie shoulder yaw");
    check(near(leg(0,1,0),leg(0,1,3)) && near(leg(0,1,1),leg(0,1,2)) && near(leg(0,1,0),-leg(0,1,1)),"quadruped and creeper diagonal gait");
    check(near(interpolate_yaw(pi-.1,-pi+.1,.5f),pi),"yaw interpolation crosses the seam via shortest arc");
    std::cout<<"1.12.2 biped/zombie/quadruped equations, pivots, easing and 20Hz interpolation passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
