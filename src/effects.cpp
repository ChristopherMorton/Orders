#include "effects.h"
#include "level.h"
#include "util.h"
#include "units.h"
#include "log.h"
#include "animation.h"
#include "menustate.h"

#include <cmath>
#include <sstream>

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

namespace sum
{

Effect::~Effect()
{

}

///////////////////////////////////////////////////////////////////////////////
// Projectile ---

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
         if (type == PR_WIND_SLASH) {
            alt_data -= dtf;
            while (alt_data < 0) {
               alt_data += 0.1;
               nearest->takeDamage( damage * 0.1, DMG_HEAVY );
            }
            return 0;
         } else {
            nearest->takeDamage( damage );
            return 1;
         }
      }
   }

   return 0;
}

bool init_projectiles = false;

Sprite *sp_magic_projectile = NULL;
Sprite *sp_arrow = NULL;
Animation anim_wind_slash;

void initProjectiles()
{
   Texture *t;
   if (NULL == sp_arrow) {
      t = SFML_TextureManager::getSingleton().getTexture( "Arrow.png" ); 
      sp_arrow = new Sprite( *(t));
      Vector2u dim = t->getSize();
      sp_arrow->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_arrow );
      sp_arrow->scale( 0.4, 0.4 );
   }

   if (NULL == sp_magic_projectile) {
      t = SFML_TextureManager::getSingleton().getTexture( "orb0.png" ); 
      sp_magic_projectile = new Sprite( *(t));
      Vector2u dim = t->getSize();
      sp_magic_projectile->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_magic_projectile );
   }

   t = SFML_TextureManager::getSingleton().getTexture( "ProjAnimWindSlash.png" );
   anim_wind_slash.load( t, 100, 100, 1, 500 );

   init_projectiles = true;
}

int Projectile::draw( )
{
   if (!init_projectiles)
      initProjectiles();

   if (!isVisible( (int)pos.x, (int)pos.y ))
      return -1;

   Sprite *sp_pr = NULL;
   if (type == PR_ARROW) sp_pr = sp_arrow;
   else if (type == PR_HOMING_ORB) sp_pr = sp_magic_projectile;
   else if (type == PR_WIND_SLASH) {
      sp_pr = anim_wind_slash.getSprite( 0 );
      sp_pr->setOrigin( 50, 50 );
      sp_pr->setScale( 0.01, 0.01 );
   }

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
      case PR_WIND_SLASH:
         radius = 0.3;
         damage = 25.0; // dps
         alt_data = 0.0;
      default:
         break;
   }
}

Projectile::~Projectile()
{

}

///////////////////////////////////////////////////////////////////////////////
// StaticEffect ---

int StaticEffect::update( float dtf )
{
   duration -= dtf;
   if (duration <= 0)
      return 1;

   return 0;
}

Sprite *sp_summon_cloud = NULL;
Sprite *sp_spear_anim = NULL;
Sprite *sp_alert_marker = NULL;
Sprite *sp_go_marker = NULL;

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
   if (NULL == sp_alert_marker) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "PlayerAlert.png" ); 
      sp_alert_marker = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_alert_marker->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_alert_marker );
      sp_alert_marker->scale( 0.5, 0.5 );
   }
   if (NULL == sp_go_marker) {
      Texture *tex =SFML_TextureManager::getSingleton().getTexture( "PlayerGoButton.png" ); 
      sp_go_marker = new Sprite( *(tex));
      Vector2u dim = tex->getSize();
      sp_go_marker->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_go_marker );
      sp_go_marker->scale( 0.5, 0.5 );
   }

   Sprite *sp = NULL;
   if (type == SE_SUMMON_CLOUD)
      sp = sp_summon_cloud;
   else if (type == SE_SPEAR_ANIM)
      sp = sp_spear_anim;
   else if (type == SE_ALERT_MARKER)
      sp = sp_alert_marker;
   else if (type == SE_GO_MARKER)
      sp = sp_go_marker;

   if (NULL != sp) {
      sp->setPosition( pos );
      sp->setRotation( rotation );

      Color c = Color::White;
      if (fade_dur > 0.05 && duration < fade_dur) {
         int alpha = 255 - (int)(((fade_dur - duration) / fade_dur) * 255);
         if (alpha < 0) alpha = 0;
         if (alpha > 255) alpha = 255;
         c = Color( 255, 255, 255, alpha );
      }
      sp->setColor( c );

      SFML_GlobalRenderWindow::get()->draw( *sp );
   }

   return 0;
}

StaticEffect::StaticEffect( Effect_Type t, float dur, float x, float y, float rot, float fade )
{
   type = t;
   duration = dur;
   fade_dur = fade;
   rotation = rot;
   pos = Vector2f( x, y );
}

StaticEffect::~StaticEffect()
{

}

///////////////////////////////////////////////////////////////////////////////
// Unique Effects ---

///////////////////////////////////////////////////////////////////////////////
// Monster Burst ---

Sprite *sp_burst_spike;
bool init_monster_burst = false;

void initMonsterBurst()
{
   Texture *t;
   if (NULL == sp_burst_spike) {
      t = SFML_TextureManager::getSingleton().getTexture( "MonsterBurstSpike.png" ); 
      sp_burst_spike = new Sprite( *(t));
      Vector2u dim = t->getSize();
      sp_burst_spike->setOrigin( dim.x / 2.0, dim.y );
      normalizeTo1x1( sp_burst_spike );
      sp_burst_spike->scale( 0.3, 0.3 );
   }

   init_monster_burst = true;
}

int MonsterBurst::update( float dtf )
{
   duration -= dtf;
   if (duration < 0)
      return 1;

   return 0;
}

int MonsterBurst::draw()
{
   if (!init_monster_burst)
      initMonsterBurst();

   // Fade at last moment
   Color c = Color::White;
   if (duration < 0.1) {
      int alpha = 255 - (int)(((0.1 - duration) / 0.1) * 255);
      if (alpha < 0) alpha = 0;
      if (alpha > 255) alpha = 255;
      c = Color( 255, 255, 255, alpha );
   }
   sp_burst_spike->setColor( c );
   sp_burst_spike->setPosition( _x, _y );
   normalizeTo1x1( sp_burst_spike );
   sp_burst_spike->scale( 0.3, 0.3 );

   Vector2u dim = SFML_TextureManager::getSingleton().getTexture( "MonsterBurstSpike.png" )->getSize();

   float offset = dim.y * (1 + ((0.25 - duration) * 12));
   sp_burst_spike->setOrigin( dim.x / 2.0, offset );

   sp_burst_spike->setRotation( 0 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 90 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 180 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 270 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );

   offset = offset * 1.3;
   sp_burst_spike->setOrigin( dim.x / 2.0, offset );

   sp_burst_spike->setRotation( 45 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 135 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 225 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 315 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );

   sp_burst_spike->setOrigin( dim.x / 2.0, offset );
   sp_burst_spike->scale( 0.5, 0.5 );

   sp_burst_spike->setRotation( 22.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 67.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 112.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 157.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 202.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 247.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 292.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );
   sp_burst_spike->setRotation( 337.5 );
   SFML_GlobalRenderWindow::get()->draw( *sp_burst_spike );

   return 0;
}

MonsterBurst::MonsterBurst( float x, float y )
{
   type = EF_MONSTER_BURST;
   _x = x;
   _y = y;
   duration = 0.2;
}

MonsterBurst::~MonsterBurst()
{ }

///////////////////////////////////////////////////////////////////////////////
// DamageDisplay ---

const float c_dmg_display_height = 0.12;
const float c_dmg_display_height_spacer = 0.04;
const float c_dmg_display_dur_half = 0.2;

int DamageDisplay::update( float dtf )
{
   duration -= dtf;
   if (duration <= 0)
      return 1;

   return 0;
}

int DamageDisplay::draw()
{
   if (attached_unit == NULL)
      return -1;

   if (duration < c_dmg_display_dur_half) {
      Color new_c = damage_text->getColor();
      new_c.a = (int) (255 * (duration / c_dmg_display_dur_half) );
      damage_text->setColor( new_c );
   }

   damage_text->setPosition( attached_unit->x_real + offset_x, attached_unit->y_real + offset_y );
   SFML_GlobalRenderWindow::get()->draw( *damage_text );
   return 0;
}

DamageDisplay::DamageDisplay( int damage, DamageType dmg_type, Unit *unit, int slot )
{
   type = EF_DAMAGE_DISPLAY;
   duration = 2 * c_dmg_display_dur_half;

   // Setup offset
   attached_unit = unit;
   attached_slot = slot;

   offset_y = ((slot % 6) * (c_dmg_display_height + c_dmg_display_height_spacer)) 
                              - (0.5 + c_dmg_display_height_spacer);
   offset_x = ((slot / 6) * 0.5) - 0.5;

   // Setup Text
   damage_text = new Text();
   damage_text->setFont( *menu_font );
   damage_text->setCharacterSize( 36 );

   stringstream ss;
   if (dmg_type == DMG_HEAL)
      ss << "+";
   else
      ss << "-";
   ss << damage;
   damage_text->setString( String( ss.str() ) );

   if (dmg_type == DMG_HEAL)
      damage_text->setColor( Color( 0, 127, 0, 255 ) );
   else
      damage_text->setColor( Color( 127, 0, 0, 255 ) );

   FloatRect dim = damage_text->getGlobalBounds();
   damage_text->setScale( c_dmg_display_height / dim.height, c_dmg_display_height / dim.height );
}

DamageDisplay::~DamageDisplay()
{ 
   if (damage_text)
      delete damage_text;

   if (attached_unit != NULL && (attached_slot >= 0 && attached_slot < 12))
      attached_unit->dmg_display[attached_slot] = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// General ---

int initEffects()
{
   // Setup Sprites
   initProjectiles();
   initMonsterBurst();

   log("initEffects complete");
   return 0;
}

Projectile *genProjectile( Effect_Type t, int tm, float x, float y, float speed, float range, Unit* target, float homing, float fastforward )
{
   Projectile *result = NULL;
   if (target && t <= PR_WIND_SLASH) {
      log("Generating projectile");
      result = new Projectile( t, tm, x, y, speed, range, target, homing );
   }

   if (fastforward > 0)
      result->update( fastforward );

   return result;
}

StaticEffect *genEffect( Effect_Type t, float dur, float x, float y, float rotation, float fade )
{
   StaticEffect *result = NULL;
   if (t >= SE_SUMMON_CLOUD && t <= SE_SPEAR_ANIM) {
      log("Generating effect");
      result = new StaticEffect( t, dur, x, y, rotation, fade );
   }

   return result;
}

};
