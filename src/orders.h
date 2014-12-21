#ifndef ORDERS_H__
#define ORDERS_H__

namespace sf { class Texture; };

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

      FOLLOW_PATH,

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

      BUMP,

      SKIP,

      // Unit specific

      // Monster
      MONSTER_GUARD,
      MONSTER_BURST,

      // Soldier
      SOLDIER_SWITCH_AXE,
      SOLDIER_SWITCH_SPEAR,
      SOLDIER_SWITCH_BOW,

      // Worm
      WORM_SPRINT,
      WORM_TRAIL_START,
      WORM_TRAIL_END,

      // Bird
      BIRD_CMD_SHOUT,
      BIRD_CMD_QUIET,
      BIRD_FLY,

      // Bug
      BUG_CAST_FIREBALL,
      BUG_CAST_SUNDER,
      BUG_CAST_HEAL,
      BUG_OPEN_WORMHOLE,
      BUG_CLOSE_WORMHOLE,
      BUG_MEDITATE,

      // Player Commands
      // Alert
      PL_ALERT_ALL,
      PL_ALERT_TEAM,
      PL_ALERT_MONSTERS,
      PL_ALERT_SOLDIERS,
      PL_ALERT_WORMS,
      PL_ALERT_BIRDS,
      PL_ALERT_BUGS,
      // Activate
      PL_CMD_GO, // Currently alert units
      PL_CMD_GO_ALL,
      PL_CMD_GO_TEAM,
      PL_CMD_GO_MONSTERS,
      PL_CMD_GO_SOLDIERS,
      PL_CMD_GO_WORMS,
      PL_CMD_GO_BIRDS,
      PL_CMD_GO_BUGS,

      // Player spells
      PL_CAST_HEAL,
      PL_CAST_LIGHTNING,
      PL_CAST_TIMELOCK,
      PL_CAST_SCRY,
      PL_CAST_CONFUSION,

      // Unit summoning
      SUMMON_MONSTER,
      SUMMON_SOLDIER,
      SUMMON_WORM,
      SUMMON_BIRD,
      SUMMON_BUG,

      FAILED_SUMMON,

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

   sf::Texture *getOrderTexture( Order o );
   void drawOrder( Order o, int x, int y, int size );
};

#endif 
