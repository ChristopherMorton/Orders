#include "units.h"
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

using namespace sf;
using namespace std;

namespace sum
{

//////////////////////////////////////////////////////////////////////
// Base Unit
//////////////////////////////////////////////////////////////////////

int Unit::startBasicOrder( Order o )
{
   if (o.action <= WAIT) {
      switch (o.action) {
         case MOVE_BACK:
         case MOVE_FORWARD:

            return 1;
         case TURN_NORTH:
            TurnTo(NORTH);
            return 0;
         case TURN_EAST:
            TurnTo(EAST);
            return 0;
         case TURN_SOUTH:
            TurnTo(SOUTH);
            return 0;
         case TURN_WEST:
            TurnTo(WEST);
            return 0;
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
            
         case ATTACK_CLOSEST:
         case ATTACK_FURTHEST:
         case ATTACK_BIGGEST:
         case ATTACK_SMALLEST:
            progress += dt;
            if (progress >= speed)
               doAttack( o );
            return 0;

         case TURN_NORTH:
         case TURN_EAST:
         case TURN_SOUTH:
         case TURN_WEST:
            log("ERROR: update on TURN_* order");
            return -2;
         case START_BLOCK:
         case END_BLOCK:
         case REPEAT:
            log("ERROR: update on control block order");
            return -2;

         case WAIT:
         default:
            return 0;
      }
   }
   else return -1;
}

int Unit::completeBasicOrder( Order o )
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

void Unit::clearOrders()
{
   order_count = 0;
   current_order = -1;
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
   if (current_order == -1 && order_count > 0)
      current_order = 0; // Start following orders

   while (current_order < order_count && current_order >= 0) {
      Order o = order_queue[current_order];
      int r = startBasicOrder(o);
      // if startBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else break;
   }

   return 0;
}

int Unit::update( int dt, float dtf )
{
   if (current_order < order_count && current_order >= 0) {
      Order o = order_queue[current_order];
      return updateBasicOrder( dt, dtf, o );
   }
   return 0;
}

int Unit::completeTurn()
{
   if (current_order < order_count && current_order >= 0) {
      Order o = order_queue[current_order];
      int r = completeBasicOrder(o);
      current_order++;
      return r;
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

   max_orders = MAGICIAN_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

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
