#include "units.h"
#include "level.h"
#include "focus.h"
#include "util.h"
#include "effects.h"
#include "animation.h"
#include "clock.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

//////////////////////////////////////////////////////////////////////
// Definitions ---

#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAX_ORDERS 500

#define MONSTER_BASE_HEALTH 600
#define MONSTER_BASE_MEMORY 10
#define MONSTER_BASE_VISION 3.5
#define MONSTER_BASE_SPEED 0.1

#define SOLDIER_BASE_HEALTH 300
#define SOLDIER_BASE_MEMORY 16
#define SOLDIER_BASE_VISION 6.5
#define SOLDIER_BASE_SPEED 0.1

#define WORM_BASE_HEALTH 40
#define WORM_BASE_MEMORY 14
#define WORM_BASE_VISION 8.5
#define WORM_BASE_SPEED 0.1

#define BIRD_BASE_HEALTH 200
#define BIRD_BASE_MEMORY 16
#define BIRD_BASE_VISION 6.5
#define BIRD_BASE_SPEED 0.1

#define BUG_BASE_HEALTH 100
#define BUG_BASE_MEMORY 14
#define BUG_BASE_VISION 5.5
#define BUG_BASE_SPEED 0.8
#define BUG_ORB_SPEED 3.0

using namespace sf;
using namespace std;

namespace sum
{

//////////////////////////////////////////////////////////////////////
// Base Unit ---

int Unit::prepareBasicOrder( Order &o, bool cond_result )
{
   int nest;

   if (o.action <= WAIT) {
      switch (o.action) {

         // MOVEMENT

         case MOVE_BACK:
            if (cond_result == true) {
               // Set new next location
               if (facing == NORTH) { TurnTo(SOUTH); facing = NORTH; }
               else if (facing == SOUTH) { TurnTo(NORTH); facing = SOUTH; }
               else if (facing == EAST) { TurnTo(WEST); facing = EAST; }
               else if (facing == WEST) { TurnTo(EAST); facing = WEST; }
            }
         case MOVE_FORWARD:
            if (cond_result == true) {
               // Can we move there? If not, reset *_next values and quit
               if (!canMove( x_next, y_next, x_grid, y_grid )) {
                  TurnTo(facing);
                  return 0;
               }

               return 1;
            } else {
               return 0;
            }
         case TURN_NORTH:
            if (cond_result == true) {
               TurnTo(NORTH);
            } else {
            }
            return 0;
         case TURN_EAST:
            if (cond_result == true) {
               TurnTo(EAST);
            } else {
            }
            return 0;
         case TURN_SOUTH:
            if (cond_result == true) {
               TurnTo(SOUTH);
            } else {
            }
            return 0;
         case TURN_WEST:
            if (cond_result == true) {
               TurnTo(WEST);
            } else {
            }
            return 0;

         // ATTACKING

         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (cond_result == true) {
               done_attack = 0;
               return 1;
            } else {
               return 0;
            }

         // CONTROL

         case START_BLOCK:
            if (cond_result == true && (o.iteration < o.count || o.count == -1)) {
               o.iteration++;
               return 0;
            } else {
               o.iteration = 0;
               nest = 1;
               for (int i = current_order + 1; i < order_count; ++i) {
                  if (order_queue[i].action == START_BLOCK)
                     nest++;

                  if (order_queue[i].action == END_BLOCK) {
                     nest--;
                     if (nest == 0) { // Found matching end block - go to it, then skip
                        current_order = i;
                        return 0;
                     }
                  }
               }
               // Should only reach here if the block isn't closed...
               log("Skipped a START_BLOCK with no matching END_BLOCK");   
               current_order = order_count;
               active = 0;
               return 1;
            }
         case END_BLOCK:
            // Find matching start block, and go to it - that's it
            nest = 1;
            for (int i = current_order - 1; i >= 0; --i) {
               if (order_queue[i].action == END_BLOCK)
                  nest++;

               if (order_queue[i].action == START_BLOCK) {
                  nest--;
                  if (nest == 0) { // Found matching start block - go 1 before it, and skip
                     current_order = i-1;
                     return 0;
                  }
               }
            }
            log("Found an END_BLOCK with no matching START_BLOCK - skipping");
            // Ignore it
            return 0;
         case REPEAT:
            if (cond_result == true && (o.iteration < o.count || o.count == -1)) {
               o.iteration++;
               current_order = -1; // Goto 0
               return 0;
            } else {
               o.iteration = 0;
               return 0;
            }

         case WAIT:
         default:
            return 1;
      }

   }
   else return -1;

}

bool testUnitCanMove( Unit *u )
{
   if (NULL == u) return false;

   bool r = canMoveUnit( u->x_next, u->y_next, u->x_grid, u->y_grid, u );
   if (false == r) {
      u->this_turn_order = Order( BUMP );
   }
   return r;
}

int Unit::startBasicOrder( )
{
   if (this_turn_order.action > WAIT) return -1;

   if (this_turn_order.action == MOVE_FORWARD || 
       this_turn_order.action == MOVE_BACK)
   {
      testUnitCanMove( this );
   }

   return 0;
}

int Unit::updateBasicOrder( float dtf, Order o )
{
   if (o.action <= WAIT) { 
      switch(o.action) {
         case MOVE_BACK:
            dtf = -dtf;
         case MOVE_FORWARD:
            if (facing == NORTH)
               y_real -= dtf;
            else if (facing == SOUTH)
               y_real += dtf;
            else if (facing == WEST)
               x_real -= dtf;
            else if (facing == EAST)
               x_real += dtf;
            return 0;

         case TURN_NORTH:
         case TURN_EAST:
         case TURN_SOUTH:
         case TURN_WEST:
            log("ERROR: update on turn order");
            return -2;
            
         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (!done_attack) {
               if (progress >= speed) {
                  doAttack( o );
               }
            }
            return 0;

         case START_BLOCK:
         case END_BLOCK:
         case REPEAT:
            log("ERROR: update on control order");
            return -2;

         case WAIT:
         default:
            return 0;
      }
   }
   else return -1;
}

int Unit::completeBasicOrder( Order &o )
{
   o.logSelf();
   if (o.action <= WAIT) {
      switch (o.action) {
         case MOVE_BACK:
         case MOVE_FORWARD:
            moveUnit( this, x_next, y_next );
            x_grid = x_next;
            y_grid = y_next;
            TurnTo(facing); 
         default: 
            return 0;
      }
   }
   else return -1;
}

bool Unit::evaluateConditional( Order o )
{
   switch (o.condition) {
      case TRUE:
         return true;
      case FALSE:
         return false;
      case ENEMY_IN_RANGE:
         return getEnemy( x_grid, y_grid, attack_range, facing, this, SELECT_CLOSEST ) != NULL;
      default:
         return true;
   }
}

void Unit::activate()
{
   current_order = 0;
   active = 2;
}

void Unit::clearOrders()
{
   order_count = 0;
   current_order = 0;
   final_order = 0;
   log("Cleared orders");
}

int Unit::TurnTo( Direction face )
{
   facing = face;

   if (face == NORTH) {
      x_next = x_grid;
      y_next = y_grid - 1;
   } else if (face == SOUTH) {
      x_next = x_grid;
      y_next = y_grid + 1;
   } else if (face == EAST) {
      x_next = x_grid + 1;
      y_next = y_grid;
   } else if (face == WEST) {
      x_next = x_grid - 1;
      y_next = y_grid;
   } else if (face == ALL_DIR) {
      return -1;
   }

   return 0;
}

Unit::~Unit()
{
   if (order_queue)
      delete order_queue;

}

void Unit::logOrders()
{
   log("Unit printOrders:");
   for( int i = 0; i < order_count; i ++ )
   {
      if (i == current_order)
         log("CURRENT ORDER:");
      if (i == final_order)
         log("FINAL ORDER:");

      order_queue[i].logSelf();
   }
}

int Unit::addOrder( Order o )
{
   if (o.action <= WAIT) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Unit::prepareTurn()
{
   if (active == 2) active = 1; // Start an active turn

   this_turn_order = Order( WAIT );

   while (active == 1 && 
          current_order != final_order) {
      this_turn_order = order_queue[current_order];
      bool decision = evaluateConditional(this_turn_order);
      int r = prepareBasicOrder(this_turn_order, decision);
      // if prepareBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else 
         break;
   }

   if (current_order == final_order)
      this_turn_order = Order( WAIT );

   progress = 0;
   return 0;
}

int Unit::startTurn()
{
   startBasicOrder();
   return 0;
}

int Unit::update( float dtf )
{
   if (!alive)
      return 1;

   progress += dtf;
   if (active == 1 && current_order != final_order) {
      Order &o = this_turn_order;
      return updateBasicOrder( dtf, o );
   }
   return 0;
}

// Run completeBasicOrder, update iterations, select next order
int Unit::completeTurn()
{
   if (active == 1 && current_order != final_order) {
      int r = completeBasicOrder(this_turn_order);
      Order &o = order_queue[current_order];
      o.iteration++;
      if (o.iteration >= o.count && o.count != -1) { 
         current_order++;
         o.iteration = 0;
      }
      return r;
   }

   return 0;
}

int Unit::takeDamage( float damage )
{
   health -= damage;
   if (health <= 0) {
      // Dead!
      alive = false;
      return -1;
   }

   return 0;
}

string Unit::descriptor()
{
   return "BASIC UNIT";
}

//////////////////////////////////////////////////////////////////////
// RangedUnit ---

// Private
RangedUnit::RangedUnit()
{ }

RangedUnit::RangedUnit( UnitType t, int x, int y, Direction face, int my_team )
{
   if ( t < R_HUMAN_ARCHER_T || t > R_HUMAN_ARCHER_T ) {
      alive = false;
      return;
   }

   alive = true;

   type = t;
   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;

   TurnTo(face);

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 1;
   team = my_team;

   switch (t) {
      case R_HUMAN_ARCHER_T:
         radius = 0.3;
         health = max_health = 100;
         vision_range = 8.5;
         attack_range = 8.5;
         speed = 0.6;
         ai_type = STAND_AND_FIRE;
         break;
      default:
         break;
   }
}

int RangedUnit::doAttack( Order o )
{
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      addProjectile( PR_ARROW, team, x_real, y_real, 3.0, attack_range, target );
   }

   done_attack = 1;
   return 0;
}

std::string RangedUnit::descriptor()
{
   return "Human Archer";
}

int RangedUnit::prepareTurn()
{
   if (ai_type == STAND_AND_FIRE) {
      this_turn_order = Order( ATTACK_CLOSEST );
      done_attack = 0;
   }

   return 0;
}

int RangedUnit::update( float dtf )
{
   if (!alive)
      return 1;

   progress += dtf;
   Order &o = this_turn_order;
   return updateBasicOrder( dtf, o );
}

sf::Texture* RangedUnit::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "HumanArcher.png" );
}

Sprite *sp_human_archer = NULL;

int RangedUnit::draw()
{
   if (NULL == sp_human_archer) {
      sp_human_archer = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_human_archer->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_human_archer );
      //sp_human_archer->scale( 0.5, 0.5 );
   }

   sp_human_archer->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_human_archer->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_human_archer );

   return 0;

}

RangedUnit::~RangedUnit()
{ }

//////////////////////////////////////////////////////////////////////
// Player ---

// Static

Player *thePlayer = NULL;

Animation player_anim_idle;

void initPlayerAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "BugScratchWiggleAnimation.png" );
   player_anim_idle.load( t, 128, 128, 3, 1000 );
}

// Private
Player::Player()
{ }

Player::~Player()
{
   type = PLAYER_T;
}

Player* Player::initPlayer( int grid_x, int grid_y, Direction facing )
{
   if (NULL == thePlayer)
      thePlayer = new Player();
   
   thePlayer->init( grid_x, grid_y, facing );
   return thePlayer;
}

int Player::init( int x, int y, Direction face )
{
   alive = true;

   radius = 0.4;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = PLAYER_MAX_HEALTH;

   vision_range = 0.5;
   attack_range = 3.5;

   speed = 0.99;

   max_orders = PLAYER_MAX_ORDERS;
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   initPlayerAnimations();

   active = 0;
   team = 0; 

   return 0;
}

int Player::addOrder( Order o )
{
   // Player uses a looping queue instead of a list
   if (order_count >= PLAYER_MAX_ORDERS) // No more memory!!
      return -2;

   order_queue[final_order] = o;
   order_count++;
   final_order++;
   if (final_order == PLAYER_MAX_ORDERS)
      final_order = 0;

   return 0;
}

int Player::doAttack( Order o )
{
   // Has no attack
   return 0;
}

int Player::prepareTurn()
{
   if (current_order == final_order) { // Nothing to do
      active = 0;
      return -1;
   }
   else active = 1;

   // Here is where we shout a new command
   Order o = order_queue[current_order];
   if (o.action < PL_ALERT_ALL)
      broadcastOrder( o );
   else {
      if (startPlayerCommand( o ) == -2)
         order_queue[current_order].action = FAILED_SUMMON;
   }

   progress = 0;
   return 0;
}

int Player::startTurn()
{

   return 0;
}

int Player::completeTurn()
{
   if (current_order == final_order) // Nothing to do
      return -1;

   if (active) { // started the turn doing something
      Order o = order_queue[current_order];
      if (o.action >= PL_ALERT_ALL)
         completePlayerCommand(o);

      do {
         current_order++;
         order_count--;
         if (current_order == PLAYER_MAX_ORDERS)
            current_order = 0; // loop
      } while (order_queue[current_order].action == SKIP);
   }

   return 0;
}

int Player::update( float dtf )
{
   if (!alive)
      return 1;

   progress += dtf;
   return 0;
}

sf::Texture* Player::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BugScratch.png" );
}

int Player::draw()
{ 
   Sprite *sp_player = player_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_player) return -1;

   Vector2u dim (player_anim_idle.image_size_x, player_anim_idle.image_size_y);
   sp_player->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   float scale_x = 1.0 / dim.x;
   float scale_y = 1.0 / dim.y;
   sp_player->setScale( scale_x, scale_y );

   sp_player->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_player->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_player );

   return 0;
}

string Player::descriptor()
{
   return "Sheherezad";
}

//////////////////////////////////////////////////////////////////////
// Monster ---

// *tors
Monster::Monster()
{
   Monster( -1, -1, SOUTH );
}

Monster::Monster( int x, int y, Direction face )
{
   log("Creating Monster");

   type = MONSTER_T;

   alive = true;

   radius = 0.5;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = MONSTER_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = MONSTER_BASE_VISION * (1.0 + ((float)focus_vision / 25.0));
   attack_range = 1.3;

   speed = MONSTER_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = MONSTER_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

Monster::~Monster()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Monster::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= MONSTER_GUARD && o.action <= MONSTER_BURST)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Monster::doAttack( Order o )
{
   log("Monster doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Monster attack");
      // TODO: Melee attack
   }

   done_attack = 1;
   return 0;
}

/*
int Monster::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Monster::completeTurn()
{

   return 0;
}

int Monster::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Monster::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "MonsterScratch.png" );
}

Sprite *sp_monster = NULL;

int Monster::draw()
{
   if (NULL == sp_monster) {
      sp_monster = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_monster->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_monster );
      //sp_monster->scale( 0.5, 0.5 );
   }

   sp_monster->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_monster->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_monster );

   return 0;
}

string Monster::descriptor()
{
   return "Allied Monster";
}

//////////////////////////////////////////////////////////////////////
// Soldier ---

// *tors
Soldier::Soldier()
{
   Soldier( -1, -1, SOUTH );
}

Soldier::Soldier( int x, int y, Direction face )
{
   log("Creating Soldier");

   type = SOLDIER_T;

   alive = true;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = SOLDIER_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = SOLDIER_BASE_VISION * (1.0 + ((float)focus_vision / 25.0));
   attack_range = 1.3;

   speed = SOLDIER_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = SOLDIER_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

Soldier::~Soldier()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Soldier::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= SOLDIER_SWITCH_AXE && o.action <= SOLDIER_SWITCH_BOW)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Soldier::doAttack( Order o )
{
   log("Soldier doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Soldier attack");
      // TODO: Melee poison attack
   }

   done_attack = 1;
   return 0;
}

/*
int Soldier::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Soldier::completeTurn()
{

   return 0;
}

int Soldier::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Soldier::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "SoldierScratch.png" );
}

Sprite *sp_soldier = NULL;

int Soldier::draw()
{
   if (NULL == sp_soldier) {
      sp_soldier = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_soldier->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_soldier );
      sp_soldier->scale( 0.7, 0.7 );
   }

   sp_soldier->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_soldier->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_soldier );

   return 0;
}

string Soldier::descriptor()
{
   return "Allied Soldier";
}


//////////////////////////////////////////////////////////////////////
// Worm ---

// *tors
Worm::Worm()
{
   Worm( -1, -1, SOUTH );
}

Worm::Worm( int x, int y, Direction face )
{
   log("Creating Worm");

   type = WORM_T;

   alive = true;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = WORM_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = WORM_BASE_VISION * (1.0 + ((float)focus_vision / 25.0));
   attack_range = 1.3;

   speed = WORM_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = WORM_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

Worm::~Worm()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Worm::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= WORM_SPRINT && o.action <= WORM_TRAIL_END)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Worm::doAttack( Order o )
{
   log("Worm doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Worm attack");
      // TODO: Melee poison attack
   }

   done_attack = 1;
   return 0;
}

/*
int Worm::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Worm::completeTurn()
{

   return 0;
}

int Worm::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Worm::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "WormScratch.png" );
}

Sprite *sp_worm = NULL;

int Worm::draw()
{
   if (NULL == sp_worm) {
      sp_worm = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_worm->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_worm );
      sp_worm->scale( 0.4, 0.4 );
   }

   sp_worm->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_worm->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_worm );

   return 0;
}

string Worm::descriptor()
{
   return "Allied Worm";
}

//////////////////////////////////////////////////////////////////////
// Bird ---

// *tors
Bird::Bird()
{
   Bird( -1, -1, SOUTH );
}

Bird::Bird( int x, int y, Direction face )
{
   log("Creating Bird");

   type = BIRD_T;

   alive = true;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = BIRD_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = BIRD_BASE_VISION * (1.0 + ((float)focus_vision / 25.0));
   attack_range = 1.3;

   speed = BIRD_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = BIRD_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

Bird::~Bird()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Bird::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= BIRD_MEMORIZE_START && o.action <= BIRD_LAND)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Bird::doAttack( Order o )
{
   log("Bird doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Bird attack");
      // TODO: Melee poison attack
   }

   done_attack = 1;
   return 0;
}

/*
int Bird::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Bird::completeTurn()
{

   return 0;
}

int Bird::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Bird::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BirdScratch.png" );
}

Sprite *sp_bird = NULL;

int Bird::draw()
{
   if (NULL == sp_bird) {
      sp_bird = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_bird->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_bird );
      sp_bird->scale( 0.8, 0.8 );
   }

   sp_bird->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_bird->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_bird );

   return 0;
}

string Bird::descriptor()
{
   return "Allied Bird";
} 

//////////////////////////////////////////////////////////////////////
// Bug ---

Animation bug_anim_idle;

void initBugAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "BugScratchWiggleAnimation.png" );
   bug_anim_idle.load( t, 128, 128, 3, 1000 );
}

// *tors
Bug::Bug()
{
   Bug( -1, -1, NORTH );
}

Bug::Bug( int x, int y, Direction face )
{
   log("Creating Bug");

   type = BUG_T;

   alive = true;

   radius = 0.4;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   TurnTo(face);

   health = max_health = BUG_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = BUG_BASE_VISION * (1.0 + ((float)focus_vision / 25.0));

   attack_range = BUG_BASE_VISION;
   orb_speed = BUG_ORB_SPEED * ( 1.0 + ( focus_speed * 0.01) );

   speed = BUG_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = BUG_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

Bug::~Bug()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Bug::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= BUG_MEDITATE && o.action <= BUG_MEDITATE)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Bug::doAttack( Order o )
{
   log("Bug doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Bug pre-generate");
      addProjectile( PR_HOMING_ORB, team, x_real, y_real, orb_speed, attack_range, target );
   }

   log("Bug finishing attack");

   done_attack = 1;
   return 0;
}

/*
int Bug::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Bug::completeTurn()
{

   return 0;
}

int Bug::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Bug::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BugScratch.png" );
}

//Sprite *sp_bug = NULL;

int Bug::draw()
{
   Sprite *sp_bug = bug_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_bug) return -1;

   Vector2u dim (bug_anim_idle.image_size_x, bug_anim_idle.image_size_y);
   sp_bug->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   float scale_x = 0.5 / dim.x;
   float scale_y = 0.5 / dim.y;
   sp_bug->setScale( scale_x, scale_y );

   sp_bug->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_bug->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_bug );

   return 0;

   /*
   if (NULL == sp_bug) {
      sp_bug = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_bug->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_bug );
      sp_bug->scale( 0.5, 0.5 );
   }

   sp_bug->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_bug->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_bug );

   return 0;
   */
}

string Bug::descriptor()
{
   return "Allied Bug";
}

//////////////////////////////////////////////////////////////////////
// SummonMarker ---

SummonMarker::SummonMarker( )
{
   type = SUMMONMARKER_T;

   x_grid = 0;
   y_grid = 0;
   x_real = x_grid + 0.5;
   y_real = y_grid + 0.5;
   TurnTo( SOUTH );

   alive = false;

   health = max_health = 0;

   vision_range = 0;
   attack_range = 0;

   radius = 0.45;

   speed = 0.5;

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
}

SummonMarker *theSummonMarker = NULL;

SummonMarker* SummonMarker::get( int x, int y )
{
   if (NULL == theSummonMarker)
      theSummonMarker = new SummonMarker();
   
   theSummonMarker->x_grid = x;
   theSummonMarker->x_real = x + 0.5;

   theSummonMarker->y_grid = y;
   theSummonMarker->y_real = y + 0.5;

   theSummonMarker->progress = 0;
   theSummonMarker->rotation = 0;

   return theSummonMarker;
}

int SummonMarker::addOrder( Order o )
{
   // Does no orders
   return 0;
}

int SummonMarker::doAttack( Order o )
{
   // Does no attacks
   return 0;
}

sf::Texture* SummonMarker::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "SummoningCircle.png" );
}

int SummonMarker::update( float dtf )
{
   progress += dtf;
   rotation += (dtf * 450);
   return 0;
}

Sprite *sp_summon_marker_outside = NULL;
Sprite *sp_summon_marker_inside = NULL;

int SummonMarker::draw()
{
   if (NULL == sp_summon_marker_inside) {
      sp_summon_marker_inside = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_summon_marker_inside->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   }
   if (NULL == sp_summon_marker_outside) {
      sp_summon_marker_outside = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_summon_marker_outside->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   }

   normalizeTo1x1( sp_summon_marker_inside );
   normalizeTo1x1( sp_summon_marker_outside );

   sp_summon_marker_inside->setPosition( x_real, y_real );
   sp_summon_marker_outside->setPosition( x_real, y_real );

   if (progress < 0.6) {
      sp_summon_marker_inside->setRotation( rotation );
      sp_summon_marker_outside->setRotation( -rotation );

      float out_scale, in_scale;
      out_scale = (5.0 - (progress/0.3)) / 5.0;
      in_scale = (0.0 + (progress/0.2)) / 5.0;

      sp_summon_marker_outside->scale( out_scale, out_scale );
      sp_summon_marker_inside->scale( in_scale, in_scale );

      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_outside );
      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_inside );

      sp_summon_marker_outside->scale( -out_scale, -out_scale );
      sp_summon_marker_inside->scale( -in_scale, -in_scale );
   }
   else
   {
      int alpha = (int) ( ((progress - 0.6)/0.4) * 256);
      Color c_yellow( 255, 255, 0, alpha );
      CircleShape cir( 0.25 );
      cir.setFillColor( c_yellow );
      cir.setOrigin( 0.25, 0.25 );
      cir.setPosition( x_real, y_real );
      SFML_GlobalRenderWindow::get()->draw( cir );

      sp_summon_marker_inside->setRotation( 270 );
      sp_summon_marker_outside->setRotation( -270 );

      sp_summon_marker_outside->scale( 0.6, 0.6 );
      sp_summon_marker_inside->scale( 0.6, 0.6 );

      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_outside );
      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_inside );

      sp_summon_marker_outside->scale( -0.6, -0.6 );
      sp_summon_marker_inside->scale( -0.6, -0.6 );
   }


   return 0;
}

SummonMarker::~SummonMarker()
{

}

string SummonMarker::descriptor()
{
   return "Summon Marker";
}

//////////////////////////////////////////////////////////////////////
// TargetPractice ---

TargetPractice::TargetPractice( int x, int y, Direction face )
{
   type = TARGETPRACTICE_T;

   x_grid = x;
   y_grid = y;
   x_real = x_grid + 0.5;
   y_real = y_grid + 0.5;
   TurnTo( face );

   alive = true;

   health = max_health = 200;

   vision_range = 0;
   attack_range = 0;

   radius = 0.45;

   speed = 0.5;

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 99;
}

int TargetPractice::addOrder( Order o )
{
   // Does no orders
   return 0;
}

int TargetPractice::doAttack( Order o )
{
   // Does no attacks
   return 0;
}

sf::Texture* TargetPractice::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "TargetPractice.png" );
}

Sprite *sp_targ = NULL;

int TargetPractice::draw()
{
   if (NULL == sp_targ) {
      sp_targ = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_targ->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_targ );
   }

   sp_targ->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *sp_targ );

   return 0;

}

TargetPractice::~TargetPractice()
{

}

string TargetPractice::descriptor()
{
   return "Target";
}

int initUnits()
{
   initBugAnimations();
   return 0;
}

};
