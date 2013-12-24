#include "units.h"
#include "level.h"
#include "focus.h"
#include "util.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

#define MAGICIAN_BASE_HEALTH 100
#define MAGICIAN_BASE_MEMORY 14
#define MAGICIAN_BASE_VISION 5.0

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
               log("MOVE");
               return 1;
            } else {
               log("MOVE SKIPPED");
               return 0;
            }
         case TURN_NORTH:
            if (cond_result == 1) {
               TurnTo(NORTH);
               log("Turn NORTH");
            } else {
               log("Turn NORTH SKIPPED"); 
            }
            return 0;
         case TURN_EAST:
            if (cond_result == 1) {
               TurnTo(EAST);
               log("Turn EAST");
            } else {
               log("Turn NORTH SKIPPED"); 
            }
            return 0;
         case TURN_SOUTH:
            if (cond_result == 1) {
               TurnTo(SOUTH);
               log("Turn SOUTH");
            } else {
               log("Turn NORTH SKIPPED"); 
            }
            return 0;
         case TURN_WEST:
            if (cond_result == 1) {
               TurnTo(WEST);
               log("Turn WEST");
            } else {
               log("Turn NORTH SKIPPED"); 
            }
            return 0;

         // ATTACKING

         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (cond_result == 1) {
               return 1;
            } else {
               return 0;
            }

         // CONTROL

         case START_BLOCK:
            if (cond_result == 1 && o.iteration < o.count) {
               log("Block START");
               o.iteration++;
               return 0;
            } else {
               log("Block SKIPPED");
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
            if (cond_result == 1 && o.iteration < o.count) {
               o.iteration++;
               log("REPEAT");
               current_order = -1; // Goto 0
               return 0;
            } else {
               o.iteration = 0;
               log("REPEAT skipped");
               return 0;
            }

         case WAIT:
         default:
            return 1;
      }

   }
   else return -1;
}

int Unit::updateBasicOrder( int dt, float dtf, Order o )
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
            progress += dt;
            if (progress >= speed)
               doAttack( o );
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
         return getEnemy( x_grid, y_grid, attack_range, facing, SELECT_CLOSEST ) != NULL;
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
         log("->Order skipped ahead!");
         current_order++;
         continue;
      }
      else break;
   }

   return 0;
}

int Unit::update( int dt, float dtf )
{
   if (active && current_order < order_count && current_order >= 0) {
      Order &o = order_queue[current_order];
      return updateBasicOrder( dt, dtf, o );
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


//////////////////////////////////////////////////////////////////////
// Magician
//////////////////////////////////////////////////////////////////////

// *tors
Magician::Magician()
{
   Magician( -1, -1, NORTH );
}

Magician::Magician( int x, int y, Direction face )
{
   log("Creating Magician");
   x_grid = x_real = x;
   y_grid = y_real = y;
   TurnTo(face);

   health = max_health = MAGICIAN_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   attack_range = MAGICIAN_BASE_VISION;

   max_orders = MAGICIAN_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   active = 0;
   team = 0;
}

Magician::~Magician()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Magician::addOrder( Order o )
{
   log("Magician add order");
   if (o.action <= WAIT || (o.action >= MAG_MEDITATE)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;

      return 0;
   } else return -1;
}

int Magician::doAttack( Order o )
{

   return 0;
}

/*
int Magician::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Magician::completeTurn()
{

   return 0;
}

int Magician::update( int dt, float dtf )
{

   return 0;
}
*/

int Magician::draw()
{
   Sprite *mag = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "Magician1.png" )));

   normalizeTo1x1( mag );
   mag->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *mag );

   return 0;
}

};