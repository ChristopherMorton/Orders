#include "orders.h"
#include "log.h"

#include <string>
#include <sstream>

namespace sum
{

void Order::initOrder( Order_Action a, Order_Conditional c, int cnt )
{
   action = a;
   condition = c;
   count = cnt;
   iteration = 0;
}

Order::Order( Order_Action a, Order_Conditional c, int cnt )
{
   initOrder( a, c, cnt );
}

Order::Order( Order_Action a, Order_Conditional c )
{
   initOrder( a, c, 1 );
}

Order::Order( Order_Action a, int cnt)
{
   initOrder( a, TRUE, cnt );
}

Order::Order( Order_Action a )
{
   initOrder( a, TRUE, 1 );
}

Order::Order()
{
   initOrder( WAIT, TRUE, 1 ); 
}

/*
Order::Order( Order &right )
{
   initOrder( right.action, right.condition, right.count );
}
*/

void Order::logSelf()
{
   std::stringstream s;
   switch(action) {
      case MOVE_FORWARD:
         s << "MOVE_FORWARD";
         break;
      case MOVE_BACK:
         s << "MOVE_BACK";
         break;
      case TURN_NORTH:
         s << "TURN_NORTH";
         break;
      case TURN_EAST:
         s << "TURN_EAST";
         break;
      case TURN_SOUTH:
         s << "TURN_SOUTH";
         break;
      case TURN_WEST:
         s << "TURN_WEST";
         break;
      case ATTACK_CLOSEST:
         s << "ATTACK_CLOSEST";
         break;
      case ATTACK_FARTHEST:
         s << "ATTACK_FARTHEST";
         break;
      case ATTACK_BIGGEST:
         s << "ATTACK_BIGGEST";
         break;
      case ATTACK_SMALLEST:
         s << "ATTACK_SMALLEST";
         break;
      case START_BLOCK:
         s << "START_BLOCK";
         break;
      case END_BLOCK:
         s << "END_BLOCK";
         break;
      case REPEAT:
         s << "REPEAT";
         break;
      case WAIT:
         s << "WAIT";
         break;
      case MAG_MEDITATE:
         s << "MAG_MEDITATE";
         break;
      case NUM_ACTIONS:
         s << "NUM_ACTIONS";
         break;
   }
   s << " - ";
   switch(condition) {
      case TRUE:
         s << "TRUE";
         break;
      case FALSE:
         s << "FALSE";
         break;
      case ENEMY_IN_RANGE:
         s << "ENEMY_IN_RANGE";
         break;
      case NUM_CONDITIONALS:
         s << "NUM_CONDITIONALS";
         break;
   }
   s <<  " - " << count << "." << iteration;

   log(s.str());
}

}
