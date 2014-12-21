#include "effects.h"
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

Effect::~Effect()
{

}

int Projectile::update( float dtf )
{
   // Homing recalibrate TODO
   range -= dtf;
   if (range <= 0)
      return 1;

   pos.x += (dtf * vel.x);
   pos.y += (dtf * vel.y);

   // Calculate collisions
   Unit *nearest = getEnemy( pos.x, pos.y, 1.0, ALL_DIR, NULL, SELECT_CLOSEST );

   if (nearest != NULL && nearest->team != team) {
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

Sprite *sp_magic_projectile = NULL;
Sprite *sp_arrow = NULL;

int Projectile::draw( )
{
   if (NULL == sp_arrow) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "Arrow.png" ); 
      sp_arrow = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_arrow->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_arrow );
      sp_arrow->scale( 0.4, 0.4 );
   }

   if (NULL == sp_magic_projectile) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "orb0.png" ); 
      sp_magic_projectile = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_magic_projectile->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_magic_projectile );
   }

   if (!isVisible( (int)pos.x, (int)pos.y ))
      return -1;

   switch (type) {
      case PR_ARROW:
         sp_arrow->setPosition( pos );
         sp_arrow->setRotation( rotation );
         SFML_GlobalRenderWindow::get()->draw( *sp_arrow );
         break;
      default:
         sp_magic_projectile->setPosition( pos );
         SFML_GlobalRenderWindow::get()->draw( *sp_magic_projectile );
         break;
   }

   return 0;
}

Projectile::Projectile( Effect_Type t, int tm, float x, float y, float speed, float r, Unit* tgt )
{
   type = t;
   team = tm;
   target = tgt;

   pos.x = x;
   pos.y = y;
   range = r / speed;

   vel.x = tgt->x_real - x;
   vel.y = tgt->y_real - y;
   rotation = atan( vel.y / vel.x ) * 180 / 3.1415926;
   // normalize
   float norm = sqrt( (vel.x * vel.x) + (vel.y * vel.y) ); // distance to target - divide by
   norm = speed / norm;
   vel.x *= norm;
   vel.y *= norm;

   switch (t) {
      case PR_ARROW:
         radius = 0.05;
         damage = 20.0;
         break;
      case PR_HOMING_ORB:
         radius = 0.1;
         damage = 30.0;
         break;
   }
}

Projectile::~Projectile()
{

}

int StaticEffect::update( float dtf )
{
   duration -= dtf;
   if (duration <= 0)
      return 1;

   return 0;
}

Sprite *sp_summon_cloud = NULL;

int StaticEffect::draw()
{
   if (NULL == sp_summon_cloud) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "FogCloudTransparent.png" ); 
      sp_summon_cloud = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_summon_cloud->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_summon_cloud );
   }

   Sprite *sp = NULL;
   if (type == SE_SUMMON_CLOUD)
      sp = sp_summon_cloud;

   if (NULL != sp) {
      sp->setPosition( pos );
      SFML_GlobalRenderWindow::get()->draw( *sp );
   }

   return 0;
}

StaticEffect::StaticEffect( Effect_Type t, float dur, float x, float y )
{
   type = t;
   duration = dur;
   pos = Vector2f( x, y );
}

StaticEffect::~StaticEffect()
{

}

int initEffects()
{
   // Setup Sprites

   log("initEffects complete");
   return 0;
}

Projectile *genProjectile( Effect_Type t, int tm, float x, float y, float speed, float range, Unit* target )
{
   Projectile *result = NULL;
   if (target && t <= PR_HOMING_ORB) {
      log("Generating projectile");
      result = new Projectile( t, tm, x, y, speed, range, target );
   }

   return result;
}

StaticEffect *genEffect( Effect_Type t, float dur, float x, float y )
{
   StaticEffect *result = NULL;
   if (t >= SE_SUMMON_CLOUD && t <= SE_SUMMON_CLOUD) {
      log("Generating effect");
      result = new StaticEffect( t, dur, x, y );
   }

   return result;
}

};
