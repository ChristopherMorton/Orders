#include "projectile.h"
#include "level.h"
#include "util.h"
#include "units.h"
#include "log.h"

#include <cmath>

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

namespace sum
{

int Projectile::update( float dtf )
{
   // Homing recalibrate TODO

   pos.x += (dtf * vel.x);
   pos.y += (dtf * vel.y);

   // Calculate collisions
   Unit *nearest = getEnemy( pos.x, pos.y, 1.0, ALL_DIR, team, SELECT_CLOSEST );

   if (nearest != NULL) {
      // Is it a hit?
      float dx = (pos.x - nearest->x_real),
            dy = (pos.y - nearest->y_real);
      float dist_sq = (dx * dx) + (dy * dy);
      float sum_radius = (radius + nearest->radius);
      if (dist_sq <= (sum_radius * sum_radius)) {
         nearest->takeDamage( damage );
         return 1;
      }
   }

   return 0;
}

int Projectile::draw( )
{
   // Need ROTATION

   Sprite *sprite = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "orb0.png" )));

   normalizeTo1x1( sprite );
   sprite->setPosition( pos );
   SFML_GlobalRenderWindow::get()->draw( *sprite );

   return 0;
}

Projectile::Projectile( Projectile_Type t, int tm, float x, float y, float speed, Unit* tgt )
{
   type = t;
   team = tm;
   target = tgt;

   pos.x = x;
   pos.y = y;

   vel.x = tgt->x_real - x;
   vel.y = tgt->y_real - y;
   // normalize
   float norm = sqrt( (vel.x * vel.x) + (vel.y * vel.y) ); // distance to target - divide by
   norm = speed / norm;
   vel.x *= norm;
   vel.y *= norm;

   switch (t) {
      case ARROW:
         radius = 0.05;
         damage = 20.0;
         break;
      case HOMING_ORB:
         radius = 0.1;
         damage = 30.0;
         break;
   }
}

int initProjectiles()
{
   // Setup Sprites

   log("initProjectiles complete");
   return 0;
}

Projectile *genProjectile( Projectile_Type t, int tm, float x, float y, float speed, Unit* target )
{
   Projectile *result = NULL;
   if (target && t < NUM_PROJECTILES) {
      log("Generating projectile");
      result = new Projectile( t, tm, x, y, speed, target );
   }

   return result;
}

};
