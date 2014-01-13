#ifndef ORDERS_H__
#define ORDERS_H__

namespace sum
{ 
   enum Order_Action
   {
      // Basic Movement
      MOVE_FORWARD,
      MOVE_BACK,
      TURN_NORTH,
      TURN_EAST,
      TURN_SOUTH,
      TURN_WEST,

      // Basic Fighting
      ATTACK_CLOSEST,
      ATTACK_FARTHEST,
      ATTACK_BIGGEST,
      ATTACK_SMALLEST,

      // Basic Control Flow
      START_BLOCK,
      END_BLOCK,
      REPEAT,

      WAIT,

      // Unit specific

      // Magician
      MAG_MEDITATE,

      // Player Commands
      PL_ALERT_ALL,
      PL_ALERT_TEAM,
      PL_ALERT_HOGS,
      PL_ALERT_WARRIORS,
      PL_ALERT_WORMS,
      PL_ALERT_BIRDS,
      PL_ALERT_BUGS,

      PL_CMD_GO,

      // Player spells
      PL_CAST_HEAL,
      PL_CAST_LIGHTNING,
      PL_CAST_QUAKE,
      PL_CAST_TIMELOCK,
      PL_CAST_SCRY,
      PL_CAST_TELEPATHY,
      PL_END_TELEPATHY,

      NUM_ACTIONS
   };

   enum Order_Conditional
   {
      TRUE,
      FALSE,
      ENEMY_IN_RANGE,

      NUM_CONDITIONALS
   };

   struct Order
   {
      Order_Action action;
      Order_Conditional condition; // Generally only attached to control flow
      int count; // How many times to repeat/sustain action
      int iteration; // Local iteration count - needed for loops

      void initOrder( Order_Action, Order_Conditional, int );
      void logSelf();

      Order( Order_Action, Order_Conditional, int );
      Order( Order_Action, Order_Conditional );
      Order( Order_Action, int );
      Order( Order_Action );
      Order();

      //Order( Order& r );
   };
};

#endif 
