#include "units.h"
#include "level.h"
#include "focus.h"
#include "util.h"
#include "projectile.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAX_ORDERS 500

#define BUG_BASE_HEALTH 100
#define BUG_BASE_MEMORY 14
#define BUG_BASE_VISION 5.0
#define BUG_BASE_SPEED 0.9
#define BUG_ORB_SPEED 3.0

using namespace sf;
using namespace std;

namespace sum
{

//////////////////////////////////////////////////////////////////////
// Base Unit
//////////////////////////////////////////////////////////////////////

int Unit::startBasicOrder( Order &o, bool cond_result )
{
   int nest;

   if (o.action <= WAIT) {
      switch (o.action) {

         // MOVEMENT

         case MOVE_BACK:
         case MOVE_FORWARD:
            if (cond_result == 1) {
               return 1;
            } else {
               return 0;
            }
         case TURN_NORTH:
            if (cond_result == 1) {
               TurnTo(NORTH);
            } else {
            }
            return 0;
         case TURN_EAST:
            if (cond_result == 1) {
               TurnTo(EAST);
            } else {
            }
            return 0;
         case TURN_SOUTH:
            if (cond_result == 1) {
               TurnTo(SOUTH);
            } else {
            }
            return 0;
         case TURN_WEST:
            if (cond_result == 1) {
               TurnTo(WEST);
            } else {
            }
            return 0;

         // ATTACKING

         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (cond_result == 1) {
               done_attack = 0;
               return 1;
            } else {
               return 0;
            }

         // CONTROL

         case START_BLOCK:
            if (cond_result == 1 && (o.iteration < o.count || o.count == -1)) {
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
            if (cond_result == 1 && (o.iteration < o.count || o.count == -1)) {
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

int Unit::updateBasicOrder( float dtf, Order o )
{
   if (o.action <= WAIT) { 
      switch(o.action) {
         case MOVE_BACK:
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
         return getEnemy( x_grid, y_grid, attack_range, facing, team, SELECT_CLOSEST ) != NULL;
      default:
         return true;
   }
}

void Unit::activate()
{
   active = 1;
}

void Unit::clearOrders()
{
   order_count = 0;
   current_order = -1;
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

      return 0;
   } else return -1;
}

int Unit::startTurn()
{
   while (active && current_order < order_count && current_order >= 0) {
      Order &o = order_queue[current_order];
      bool decision = evaluateConditional(o);
      int r = startBasicOrder(o, decision);
      // if startBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else break;
   }

   progress = 0;
   return 0;
}

int Unit::update( float dtf )
{
   if (!alive)
      return 1;

   progress += dtf;
   if (active && current_order < order_count && current_order >= 0) {
      Order &o = order_queue[current_order];
      return updateBasicOrder( dtf, o );
   }
   return 0;
}

// Run completeBasicOrder, update iterations, select next order
int Unit::completeTurn()
{
   if (active && current_order < order_count && current_order >= 0) {
      Order &o = order_queue[current_order];
      int r = completeBasicOrder(o);
      o.iteration++;
      if (o.iteration >= o.count && o.count != -1) { 
         current_order++;
         o.iteration = 0;
      }
      return r;
   }

   // Prepare to start
   if (current_order == -1 && active && order_count > 0) {
      current_order = 0;
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

//////////////////////////////////////////////////////////////////////
// Player
//////////////////////////////////////////////////////////////////////

// Private
Player::Player()
{ }

Player::~Player()
{ 

}

Player *thePlayer = NULL;

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

   x_grid = x_real = x;
   y_grid = y_real = y;
   TurnTo(face);

   health = max_health = PLAYER_MAX_HEALTH;

   attack_range = 0;

   speed = 0.99;

   max_orders = PLAYER_MAX_ORDERS;
   order_queue = new Order[max_orders];
   clearOrders();

   active = 0;
   team = 0; 

   return 0;
}

int Player::addOrder( Order o )
{
   if (order_count >= max_orders) // No more memory
      return -2;

   order_queue[order_count] = o;
   order_count++;

   return 0;
}

int Player::doAttack( Order o )
{
   // Has no attack
   return 0;
}

int Player::startTurn()
{
   // Here is where we shout a new command
   Order o = order_queue[current_order];
   if (o.action < PL_ALERT_ALL)
      broadcastOrder( order_queue[current_order] );
   else
      playerCommand(o);

   progress = 0;
   return 0;
}

int Player::completeTurn()
{
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
   return SFML_TextureManager::getSingleton().getTexture( "Magician1.png" );
}

int Player::draw()
{
   Sprite *player = new Sprite(*getTexture());
 
   normalizeTo1x1( player );
   player->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *player );

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Bug
//////////////////////////////////////////////////////////////////////

// *tors
Bug::Bug()
{
   Bug( -1, -1, NORTH );
}

Bug::Bug( int x, int y, Direction face )
{
   log("Creating Bug");

   alive = true;

   radius = 0.4;

   x_grid = x_real = x;
   y_grid = y_real = y;
   TurnTo(face);

   health = max_health = BUG_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   attack_range = BUG_BASE_VISION;
   orb_speed = BUG_ORB_SPEED * ( 1.0 + ( focus_speed * 0.01) );

   speed = BUG_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = BUG_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

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
   if (o.action <= WAIT || (o.action >= MAG_MEDITATE)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;

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

   target = getEnemy( x_grid, y_grid, attack_range, facing, team, selector );

   if (target) {
      log("Bug pre-generate");
      addProjectile( HOMING_ORB, team, x_real, y_real, orb_speed, target );
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
   return SFML_TextureManager::getSingleton().getTexture( "Magician1.png" );
}

int Bug::draw()
{
   Sprite *bug = new Sprite(*getTexture());
 
   normalizeTo1x1( bug );
   bug->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *bug );

   return 0;
}

//////////////////////////////////////////////////////////////////////
// TargetPractice
//////////////////////////////////////////////////////////////////////

TargetPractice::TargetPractice( int x, int y, Direction face )
{
   x_grid = x_real = x;
   y_grid = y_real = y;
   TurnTo( face );

   alive = true;

   health = max_health = 200;

   attack_range = 0;

   radius = 0.45;

   speed = 0.5;

   max_orders = 1;
   order_queue = NULL;
   clearOrders();

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

int TargetPractice::draw()
{
   Sprite *targ = new Sprite(*getTexture());

   normalizeTo1x1( targ );
   targ->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *targ );

   return 0;

}

TargetPractice::~TargetPractice()
{

}

};
