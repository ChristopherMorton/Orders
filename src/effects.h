#ifndef EFFECTS_H__
#define EFFECTS_H__

#include "types.h"

#include <SFML/System/Vector2.hpp>

namespace sf { struct Text; };

namespace sum
{

class Unit;

enum Effect_Type
{
   // Projectiles
   PR_ARROW,
   PR_HOMING_ORB,
   PR_WIND_SLASH,
   // General Effects
   SE_SUMMON_CLOUD,
   SE_ALERT_MARKER,
   SE_GO_MARKER,
   // Combat Effects
   SE_SPEAR_ANIM,
   EF_MONSTER_BURST,
   // Other Effects
   EF_DAMAGE_DISPLAY,

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
   float duration, fade_dur;

   virtual int update( float dtf );
   virtual int draw();

   StaticEffect( Effect_Type t, float duration, float x, float y, float rotation, float fade = 0.0 );

   virtual ~StaticEffect();
};

struct MonsterBurst : public Effect
{
   float _x, _y;
   float duration;

   virtual int update( float dtf );
   virtual int draw();

   MonsterBurst( float x, float y );
   virtual ~MonsterBurst();
};

struct DamageDisplay : public Effect
{
   Unit *attached_unit;
   DamageDisplay *next;
   float offset_x, offset_y;
   sf::Text *damage_text;
   float duration;

   int subtractOffset();

   virtual int update( float dtf );
   virtual int draw();

   DamageDisplay( Unit *u, int damage, DamageType type );

   virtual ~DamageDisplay();
};

Projectile *genProjectile( Effect_Type t, int team, float x, float y, float speed, float range, Unit* target, float homing = 0, float fastforward = 0 );
StaticEffect *genEffect( Effect_Type t, float dur, float x, float y, float rotation, float fade = 0.0 );

int initEffects();

};

#endif
