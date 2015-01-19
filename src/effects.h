#ifndef EFFECTS_H__
#define EFFECTS_H__

#include <SFML/System/Vector2.hpp>

namespace sum
{

class Unit;

enum Effect_Type
{
   // Projectiles
   PR_ARROW,
   PR_HOMING_ORB,
   // Sparkly Effects
   SE_SUMMON_CLOUD,
   // Combat Effects
   SE_SPEAR_ANIM,

   NUM_EFFECTS
};

struct Effect
{
   Effect_Type type;

   virtual int update( float dtf ) = 0;
   virtual int draw() = 0;

   virtual ~Effect();
};

struct Projectile : public Effect
{
   float radius, damage;
   sf::Vector2f pos, vel;
   float range, rotation, speed;
   int team;
   Unit *target;
   float homing; // distance during which the attack will home in

   virtual int update( float dtf );
   virtual int draw();

   Projectile( Effect_Type t, int team, float x, float y, float speed, float range, Unit* target, float homing = 0 );

   virtual ~Projectile();
};

struct StaticEffect : public Effect
{
   sf::Vector2f pos;
   float rotation;
   float duration;

   virtual int update( float dtf );
   virtual int draw();

   StaticEffect( Effect_Type t, float duration, float x, float y, float rotation );

   virtual ~StaticEffect();
};

Projectile *genProjectile( Effect_Type t, int team, float x, float y, float speed, float range, Unit* target, float homing = 0, float fastforward = 0 );
StaticEffect *genEffect( Effect_Type t, float dur, float x, float y, float rotation );

int initEffects();

};

#endif
