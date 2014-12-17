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
   int team;
   Unit *target;

   virtual int update( float dtf );
   virtual int draw();

   // Speed means units travelled per turn
   Projectile( Effect_Type t, int team, float x, float y, float speed, Unit* target );

   virtual ~Projectile();
};

struct StaticEffect : public Effect
{
   sf::Vector2f pos;
   float duration;

   virtual int update( float dtf );
   virtual int draw();

   StaticEffect( Effect_Type t, float duration, float x, float y );

   virtual ~StaticEffect();
};

// Speed means units travelled per turn
Projectile *genProjectile( Effect_Type t, int team, float x, float y, float speed, Unit* target );
StaticEffect *genEffect( Effect_Type t, float dur, float x, float y );

int initEffects();

};

#endif
