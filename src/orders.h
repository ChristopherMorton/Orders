#ifndef ORDERS_H__
#define ORDERS_H__

namespace sum
{ 
   enum Order_Action
   {
      // Basic Movement
      MOVE_FORWARD,
      MOVE_BACK,
      TURN_RIGHT,
      TURN_LEFT,

      // Basic Targetting

      // Basic Fighting

      // Basic Control Flow

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
   };
};

#endif 
