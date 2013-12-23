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
      ATTACK_FURTHEST,
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

      NUM_ACTIONS
   };

   enum Order_Conditional
   {
      TRUE,
      FALSE,

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
