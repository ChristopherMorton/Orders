#ifndef PROJECTILE_H__
#define PROJECTILE_H__

#include <SFML/System/Vector2.hpp>

namespace sum
{
   class Unit;

   enum Projectile_Type
   {
      ARROW,
      HOMING_ORB,
      NUM_PROJECTILES
   };

   struct Projectile
   {
      Projectile_Type type;
      float radius;
      sf::Vector2f pos, vel;
      int team;
      Unit *target;

      int update( float dtf );
      int draw();

      // Speed means units travelled per turn
      Projectile( Projectile_Type t, int team, float x, float y, float speed, Unit* target );
   };

   // Speed means units travelled per turn
   Projectile *genProjectile( Projectile_Type t, int team, float x, float y, float speed, Unit* target );


   int loadProjectiles();
};

#endif
