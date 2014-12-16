#include "menustate.h"
#include "orders.h"
#include "log.h"
#include "util.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

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
      case FOLLOW_PATH:
         s << "FOLLOW_PATH";
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
      case BUMP:
         s << "BUMP";
         break;
      case SKIP:
         s << "SKIP";
         break;
      case MONSTER_GUARD:
         s << "MONSTER_GUARD";
         break;
      case MONSTER_BURST:
         s << "MONSTER_BURST";
         break;
      case SOLDIER_SWITCH_AXE:
         s << "SOLDIER_SWITCH_AXE";
         break;
      case SOLDIER_SWITCH_SPEAR:
         s << "SOLDIER_SWITCH_SPEAR";
         break;
      case SOLDIER_SWITCH_BOW:
         s << "SOLDIER_SWITCH_BOW";
         break;
      case WORM_SPRINT:
         s << "WORM_SPRINT";
         break;
      case WORM_TRAIL_START:
         s << "WORM_TRAIL_START";
         break;
      case WORM_TRAIL_END:
         s << "WORM_TRAIL_END";
         break;
      case BIRD_MEMORIZE_START:
         s << "BIRD_MEMORIZE_START";
         break;
      case BIRD_MEMORIZE_END:
         s << "BIRD_MEMORIZE_END";
         break;
      case BIRD_SHOUT:
         s << "BIRD_SHOUT";
         break;
      case BIRD_FLY:
         s << "BIRD_FLY";
         break;
      case BIRD_LAND:
         s << "BIRD_LAND";
         break;
      case BUG_CAST_SUNDER:
         s << "BUG_CAST_SUNDER";
         break;
      case BUG_CAST_FIREBALL:
         s << "BUG_CAST_FIREBALL";
         break;
      case BUG_CAST_HEAL:
         s << "BUG_CAST_HEAL";
         break;
      case BUG_OPEN_WORMHOLE:
         s << "BUG_OPEN_WORMHOLE";
         break;
      case BUG_CLOSE_WORMHOLE:
         s << "BUG_CLOSE_WORMHOLE";
         break;
      case BUG_MEDITATE:
         s << "BUG_MEDITATE";
         break;
      case PL_ALERT_ALL:
         s << "PL_ALERT_ALL";
         break;
      case PL_ALERT_TEAM:
         s << "PL_ALERT_TEAM";
         break;
      case PL_ALERT_MONSTERS:
         s << "PL_ALERT_MONSTERS";
         break;
      case PL_ALERT_SOLDIERS:
         s << "PL_ALERT_SOLDIERS";
         break;
      case PL_ALERT_WORMS:
         s << "PL_ALERT_WORMS";
         break;
      case PL_ALERT_BIRDS:
         s << "PL_ALERT_BIRDS";
         break;
      case PL_ALERT_BUGS:
         s << "PL_ALERT_BUGS";
         break;
      case PL_CMD_GO:
         s << "PL_CMD_GO";
         break;
      case PL_CMD_GO_ALL:
         s << "PL_CMD_GO_ALL";
         break;
      case PL_CMD_GO_TEAM:
         s << "PL_CMD_GO_TEAM";
         break;
      case PL_CMD_GO_MONSTERS:
         s << "PL_CMD_GO_MONSTERS";
         break;
      case PL_CMD_GO_SOLDIERS:
         s << "PL_CMD_GO_SOLDIERS";
         break;
      case PL_CMD_GO_WORMS:
         s << "PL_CMD_GO_WORMS";
         break;
      case PL_CMD_GO_BIRDS:
         s << "PL_CMD_GO_BIRDS";
         break;
      case PL_CMD_GO_BUGS:
         s << "PL_CMD_GO_BUGS";
         break;
      case PL_CAST_HEAL:
         s << "PL_CAST_HEAL";
         break;
      case PL_CAST_LIGHTNING:
         s << "PL_CAST_LIGHTNING";
         break;
      case PL_CAST_TIMELOCK:
         s << "PL_CAST_TIMELOCK";
         break;
      case PL_CAST_SCRY:
         s << "PL_CAST_SCRY";
         break;
      case PL_CAST_TELEPATHY:
         s << "PL_CAST_TELEPATHY";
         break;
      case PL_END_TELEPATHY:
         s << "PL_END_TELEPATHY";
         break;
      case SUMMON_MONSTER:
         s << "SUMMON_MONSTER";
         break;
      case SUMMON_SOLDIER:
         s << "SUMMON_SOLDIER";
         break;
      case SUMMON_WORM:
         s << "SUMMON_WORM";
         break;
      case SUMMON_BIRD:
         s << "SUMMON_BIRD";
         break;
      case SUMMON_BUG:
         s << "SUMMON_BUG";
         break;
      case FAILED_SUMMON:
         s << "FAILED_SUMMON";
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
   s <<  " - " << count << "(" << iteration << ")";

   log(s.str());
}

Texture *getOrderTexture( Order o )
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();

   Texture *base_tex = NULL;
   switch (o.action)
   {
      case MOVE_FORWARD:
         base_tex = t_manager.getTexture( "OrderMoveForward.png" );
         break;
      case MOVE_BACK:
         base_tex = t_manager.getTexture( "OrderMoveBackward.png" );
         break;
      case TURN_NORTH:
         base_tex = t_manager.getTexture( "OrderTurnNorth.png" );
         break;
      case TURN_EAST:
         base_tex = t_manager.getTexture( "OrderTurnEast.png" );
         break;
      case TURN_SOUTH:
         base_tex = t_manager.getTexture( "OrderTurnSouth.png" );
         break;
      case TURN_WEST:
         base_tex = t_manager.getTexture( "OrderTurnWest.png" );
         break;

      case ATTACK_CLOSEST:
         base_tex = t_manager.getTexture( "OrderAttackClosest.png" );
         break;
      case ATTACK_FARTHEST:
         base_tex = t_manager.getTexture( "OrderAttackFarthest.png" );
         break;
      case ATTACK_BIGGEST:
         base_tex = t_manager.getTexture( "OrderAttackBiggest.png" );
         break;
      case ATTACK_SMALLEST:
         base_tex = t_manager.getTexture( "OrderAttackSmallest.png" );
         break;

      case START_BLOCK:
         base_tex = t_manager.getTexture( "ControlStartBlock.png" );
         break;
      case END_BLOCK:
         base_tex = t_manager.getTexture( "ControlEndBlock.png" );
         break;
      case REPEAT:
         base_tex = t_manager.getTexture( "ControlRepeat.png" );
         break;

      case WAIT:
         base_tex = t_manager.getTexture( "OrderWait.png" );
         break;

      case PL_ALERT_ALL:
         base_tex = t_manager.getTexture( "PlayerAlert.png" );
         break;
      case PL_ALERT_TEAM:
         base_tex = t_manager.getTexture( "PlayerAlert.png" );
         break;
      case PL_ALERT_MONSTERS:
         base_tex = t_manager.getTexture( "MonsterAlertButton.png" );
         break;
      case PL_ALERT_SOLDIERS:
         base_tex = t_manager.getTexture( "SoldierAlertButton.png" );
         break;
      case PL_ALERT_WORMS:
         base_tex = t_manager.getTexture( "WormAlertButton.png" );
         break;
      case PL_ALERT_BIRDS:
         base_tex = t_manager.getTexture( "BirdAlertButton.png" );
         break;
      case PL_ALERT_BUGS:
         base_tex = t_manager.getTexture( "BugAlertButton.png" );
         break;

      case PL_CMD_GO:
         base_tex = t_manager.getTexture( "PlayerGoButton.png" );
         break;
      case PL_CMD_GO_ALL:
         base_tex = t_manager.getTexture( "PlayerGoAllButton.png" );
         break;
      case PL_CMD_GO_MONSTERS:
         base_tex = t_manager.getTexture( "MonsterGoButton.png" );
         break;
      case PL_CMD_GO_SOLDIERS:
         base_tex = t_manager.getTexture( "SoldierGoButton.png" );
         break;
      case PL_CMD_GO_WORMS:
         base_tex = t_manager.getTexture( "WormGoButton.png" );
         break;
      case PL_CMD_GO_BIRDS:
         base_tex = t_manager.getTexture( "BirdGoButton.png" );
         break;
      case PL_CMD_GO_BUGS:
         base_tex = t_manager.getTexture( "BugGoButton.png" );
         break;

      case PL_CAST_HEAL:
         base_tex = t_manager.getTexture( "CastHeal.png" );
         break;
      case PL_CAST_LIGHTNING:
         base_tex = t_manager.getTexture( "CastLightning.png" );
         break;
      case PL_CAST_TIMELOCK:
         base_tex = t_manager.getTexture( "CastTimelock.png" );
         break;
      case PL_CAST_SCRY:
         base_tex = t_manager.getTexture( "CastScry.png" );
         break;
      case PL_CAST_TELEPATHY:
         base_tex = t_manager.getTexture( "CastTelepathy.png" );
         break;
      case PL_END_TELEPATHY:
         base_tex = t_manager.getTexture( "CastEndTelepathy.png" );
         break;

      case SUMMON_MONSTER:
         base_tex = t_manager.getTexture( "SummonMonster.png" );
         break;
      case SUMMON_SOLDIER:
         base_tex = t_manager.getTexture( "SummonSoldier.png" );
         break;
      case SUMMON_WORM:
         base_tex = t_manager.getTexture( "SummonWorm.png" );
         break;
      case SUMMON_BIRD:
         base_tex = t_manager.getTexture( "SummonBird.png" );
         break;
      case SUMMON_BUG:
         base_tex = t_manager.getTexture( "SummonBug.png" );
         break;

      case FAILED_SUMMON:
      case SKIP:
      default:
         base_tex = NULL;

   }
   return base_tex;
}

Texture *getOrderButtonTexture( Order o )
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();

   switch (o.action)
   {
      case MOVE_FORWARD:
      case MOVE_BACK:
      case TURN_NORTH:
      case TURN_EAST:
      case TURN_SOUTH:
      case TURN_WEST: 
      case FOLLOW_PATH:
      case ATTACK_CLOSEST:
      case ATTACK_FARTHEST:
      case ATTACK_BIGGEST:
      case ATTACK_SMALLEST:
      case WAIT:
         return t_manager.getTexture( "OrderButtonBase.png" );

      case START_BLOCK:
      case END_BLOCK:
      case REPEAT:
         return t_manager.getTexture( "ControlButtonBase.png" );

      // Monster
      case MONSTER_GUARD:
      case MONSTER_BURST:
         return t_manager.getTexture( "MonsterOrderButtonBase.png" );

      // Soldier
      case SOLDIER_SWITCH_AXE:
      case SOLDIER_SWITCH_SPEAR:
      case SOLDIER_SWITCH_BOW:
         return t_manager.getTexture( "SoldierOrderButtonBase.png" );

      // Worm
      case WORM_SPRINT:
      case WORM_TRAIL_START:
      case WORM_TRAIL_END:
         return t_manager.getTexture( "WormOrderButtonBase.png" );

      // Bird
      case BIRD_MEMORIZE_START:
      case BIRD_MEMORIZE_END:
      case BIRD_SHOUT:
      case BIRD_FLY:
      case BIRD_LAND:
         return t_manager.getTexture( "BirdOrderButtonBase.png" );

      // Bug
      case BUG_CAST_SUNDER:
      case BUG_CAST_FIREBALL:
      case BUG_CAST_HEAL:
      case BUG_OPEN_WORMHOLE:
      case BUG_CLOSE_WORMHOLE:
      case BUG_MEDITATE:
         return t_manager.getTexture( "BugOrderButtonBase.png" );

      // Player spells
      case PL_CAST_HEAL:
      case PL_CAST_LIGHTNING:
      case PL_CAST_TIMELOCK:
      case PL_CAST_SCRY:
      case PL_CAST_TELEPATHY:
      case PL_END_TELEPATHY:
         return t_manager.getTexture( "CastButtonBase.png" );

      default:
         return NULL;
   };
}

void drawOrder( Order o, int x, int y, int size )
{
   // Base order
   Texture *base_tex = getOrderTexture( o );
   if (NULL == base_tex)
      return;

   Texture *button_tex = getOrderButtonTexture( o );
   if (NULL != button_tex) {
      // Requires a backing button

      Sprite sp_button( *button_tex );
      normalizeTo1x1( &sp_button );
      sp_button.scale( size, size );
      sp_button.setPosition( x, y );

      SFML_GlobalRenderWindow::get()->draw( sp_button );
   }


   Sprite sp_order( *base_tex );
   normalizeTo1x1( &sp_order );
   sp_order.scale( size, size );
   sp_order.setPosition( x, y );
   SFML_GlobalRenderWindow::get()->draw( sp_order );



   // Count
   if (o.count != 1 && o.action < PL_ALERT_ALL)
   {

      Text count_text;
      stringstream ss;
      ss << o.count;
      count_text.setString( String(ss.str()) );
      count_text.setFont( *menu_font );
      count_text.setColor( Color::Black );
      count_text.setCharacterSize( 10 );
      FloatRect text_size = count_text.getGlobalBounds();
      float text_x = (size - text_size.width) + x,
            text_y = (size - text_size.height) + y;
      count_text.setPosition( text_x, text_y );

      RectangleShape count_rect;
      count_rect.setSize( Vector2f((text_size.width), (text_size.height + 2)) );
      count_rect.setPosition( text_x - 1, text_y - 1 );
      count_rect.setFillColor( Color::White );
      count_rect.setOutlineColor( Color::Black );
      count_rect.setOutlineThickness( 1.0 );

      SFML_GlobalRenderWindow::get()->draw( count_rect );
      SFML_GlobalRenderWindow::get()->draw( count_text );
   }

   // Condition
   if (o.condition != TRUE && o.action < PL_ALERT_ALL) {

   }
}

};
