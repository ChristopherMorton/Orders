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
   range -= dtf;
   if (range <= 0)
      return 1;

   pos.x += (dtf * vel.x);
   pos.y += (dtf * vel.y);

   // Homing recalibrate
   if (target->alive != 1) homing = -1;

   if (homing > 0.0) {
      vel.x = target->x_real - pos.x;
      if (vel.x == 0) vel.x = 0.0001;
      vel.y = target->y_real - pos.y;
      rotation = atan( vel.y / vel.x ) * 180.0 / 3.1415926;
      if (vel.x < 0) rotation += 180;
      // normalize
      float norm = sqrt( (vel.x * vel.x) + (vel.y * vel.y) ); // distance to target - divide by
      norm = speed / norm;
      vel.x *= norm;
      vel.y *= norm;

      homing -= dtf;
   }

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

   Sprite *sp_pr = NULL;
   if (type == PR_ARROW) sp_pr = sp_arrow;
   else if (type == PR_HOMING_ORB) sp_pr = sp_magic_projectile;

   if (NULL == sp_pr) return -2;

   sp_pr->setPosition( pos );
   sp_pr->setRotation( rotation );
   SFML_GlobalRenderWindow::get()->draw( *sp_pr );

   return 0;
}

Projectile::Projectile( Effect_Type t, int tm, float x, float y, float sp, float r, Unit* tgt, float home )
{
   type = t;
   team = tm;
   target = tgt;

   pos.x = x;
   pos.y = y;
   speed = sp;
   range = r / speed;

   vel.x = tgt->x_real - x;
   if (vel.x == 0) vel.x = 0.0001;
   vel.y = tgt->y_real - y;
   rotation = atan( vel.y / vel.x ) * 180 / 3.1415926;
   if (vel.x < 0) rotation += 180;
   // normalize
   float norm = sqrt( (vel.x * vel.x) + (vel.y * vel.y) ); // distance to target - divide by
   norm = speed / norm;
   vel.x *= norm;
   vel.y *= norm;

   homing = home;

   switch (t) {
      case PR_ARROW:
         radius = 0.05;
         damage = 20.0;
         break;
      case PR_HOMING_ORB:
         radius = 0.1;
         damage = 30.0;
         break;
      default:
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
Sprite *sp_spear_anim = NULL;

int StaticEffect::draw()
{
   if (NULL == sp_summon_cloud) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "FogCloudTransparent.png" ); 
      sp_summon_cloud = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_summon_cloud->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_summon_cloud );
   }
   if (NULL == sp_spear_anim) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "SpearAttackVisualEffect.png" ); 
      sp_spear_anim = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_spear_anim->setOrigin( 0, dim.y / 2.0 );
      normalizeTo1x1( sp_spear_anim );
      sp_spear_anim->scale( 3.0, 1.0 );
   }

   Sprite *sp = NULL;
   if (type == SE_SUMMON_CLOUD)
      sp = sp_summon_cloud;
   else if (type == SE_SPEAR_ANIM)
      sp = sp_spear_anim;

   if (NULL != sp) {
      sp->setPosition( pos );
      sp->setRotation( rotation );
      SFML_GlobalRenderWindow::get()->draw( *sp );
   }

   return 0;
}

StaticEffect::StaticEffect( Effect_Type t, float dur, float x, float y, float rot )
{
   type = t;
   duration = dur;
   rotation = rot;
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

Projectile *genProjectile( Effect_Type t, int tm, float x, float y, float speed, float range, Unit* target, float homing, float fastforward )
{
   Projectile *result = NULL;
   if (target && t <= PR_HOMING_ORB) {
      log("Generating projectile");
      result = new Projectile( t, tm, x, y, speed, range, target, homing );
   }

   if (fastforward > 0)
      result->update( fastforward );

   return result;
}

StaticEffect *genEffect( Effect_Type t, float dur, float x, float y, float rotation )
{
   StaticEffect *result = NULL;
   if (t >= SE_SUMMON_CLOUD && t <= SE_SPEAR_ANIM) {
      log("Generating effect");
      result = new StaticEffect( t, dur, x, y, rotation );
   }

   return result;
}

};
