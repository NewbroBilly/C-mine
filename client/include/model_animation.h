#pragma once
#include <algorithm>
#include <cmath>

// C++ adaptation of the locally supplied 1.12.2 ModelBiped, ModelZombie,
// ModelQuadruped/ModelCreeper and EntityLivingBase limb animation calculations.
// Angles are radians; points are ModelRenderer pixels, not world units.
namespace ModelAnimation {
constexpr float pi=3.14159265358979323846f;
struct Part {float x=0,y=0,z=0,px=0,py=0,pz=0;};
struct Biped {Part head,body,right_arm,left_arm,right_leg,left_leg;};
struct Walk {
    float phase=0,amount=0,previous_amount=0,previous_phase=0;
    void tick(double dx,double dz) {
        previous_phase=phase;previous_amount=amount;
        float target=std::min(1.f,float(std::hypot(dx,dz)*4.));
        amount+=(target-amount)*.4f;phase+=amount;
    }
    float render_phase(float partial) const {return phase-amount*(1-std::clamp(partial,0.f,1.f));}
    float render_amount(float partial) const {return previous_amount+(amount-previous_amount)*std::clamp(partial,0.f,1.f);}
};
inline Biped biped(float phase,float amount,float age,float pitch,float attack,bool sneak=false,bool held=false,bool blocking=false,bool bow=false) {
    amount=std::clamp(amount,0.f,1.f);attack=std::clamp(attack,0.f,1.f);
    Biped p;p.head.x=pitch;
    p.right_arm={std::cos(phase*.6662f+pi)*amount,0,0,-5,2,0};
    p.left_arm={std::cos(phase*.6662f)*amount,0,0,5,2,0};
    p.right_leg={std::cos(phase*.6662f)*1.4f*amount,0,0,-1.9f,12,.1f};
    p.left_leg={std::cos(phase*.6662f+pi)*1.4f*amount,0,0,1.9f,12,.1f};
    if(held)p.right_arm.x=p.right_arm.x*.5f-pi/10;
    if(blocking){p.right_arm.x=p.right_arm.x*.5f-.9424779f;p.right_arm.y=-.5235988f;}
    if(attack>0) {
        p.body.y=std::sin(std::sqrt(attack)*pi*2)*.2f;
        p.right_arm.pz=std::sin(p.body.y)*5;p.right_arm.px=-std::cos(p.body.y)*5;
        p.left_arm.pz=-std::sin(p.body.y)*5;p.left_arm.px=std::cos(p.body.y)*5;
        p.right_arm.y+=p.body.y*3;p.left_arm.y+=p.body.y;p.left_arm.x+=p.body.y;
        float eased=1-std::pow(1-attack,4.f);
        float head=std::sin(attack*pi)*-(pitch-.7f)*.75f;
        p.right_arm.x-=std::sin(eased*pi)*1.2f+head;
        p.right_arm.z-=std::sin(attack*pi)*.4f;
    }
    if(sneak) {
        p.body.x=.5f;p.right_arm.x+=.4f;p.left_arm.x+=.4f;
        p.right_leg.py=p.left_leg.py=9;p.right_leg.pz=p.left_leg.pz=4;p.head.py=1;
    }
    float sway=std::cos(age*.09f)*.05f+.05f,bob=std::sin(age*.067f)*.05f;
    p.right_arm.z+=sway;p.left_arm.z-=sway;p.right_arm.x+=bob;p.left_arm.x-=bob;
    // ModelBiped applies BOW_AND_ARROW after its idle/attack calculations.
    if(bow){p.right_arm.y=-.1f;p.left_arm.y=.5f;p.right_arm.x=p.left_arm.x=-pi/2+pitch;}
    return p;
}
inline Biped zombie(float phase,float amount,float age,float attack,bool raised) {
    Biped p=biped(phase,amount,age,0,attack);
    float f=std::sin(attack*pi),f1=std::sin((1-(1-attack)*(1-attack))*pi);
    float base=-pi/(raised?1.5f:2.25f)+f*1.2f-f1*.4f;
    p.right_arm.x=base+std::sin(age*.067f)*.05f;p.left_arm.x=base-std::sin(age*.067f)*.05f;
    p.right_arm.y=-(.1f-f*.6f);p.left_arm.y=.1f-f*.6f;
    p.right_arm.z=std::cos(age*.09f)*.05f+.05f;p.left_arm.z=-p.right_arm.z;
    return p;
}
inline float leg(float phase,float amount,int index) {
    return std::cos(phase*.6662f+((index==1 || index==2)?pi:0))*1.4f*std::clamp(amount,0.f,1.f);
}
inline float interpolate_yaw(double previous,double current,float partial) {
    double delta=std::remainder(current-previous,2.*pi);
    return float(previous+delta*std::clamp(partial,0.f,1.f));
}
}
