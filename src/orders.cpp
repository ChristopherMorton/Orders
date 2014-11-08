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
         base_tex = t_manager.getTexture( "PlayerAlertRed.png" );
         break;
      case PL_ALERT_SOLDIERS:
         base_tex = t_manager.getTexture( "PlayerAlertGold.png" );
         break;
      case PL_ALERT_WORMS:
         base_tex = t_manager.getTexture( "PlayerAlertGreen.png" );
         break;
      case PL_ALERT_BIRDS:
         base_tex = t_manager.getTexture( "PlayerAlertBlue.png" );
         break;
      case PL_ALERT_BUGS:
         base_tex = t_manager.getTexture( "PlayerAlertViolet.png" );
         break;

      case PL_CMD_GO:
         base_tex = t_manager.getTexture( "PlayerGoButton.png" );
         break;
      case PL_CMD_GO_ALL:
         base_tex = t_manager.getTexture( "PlayerGoAllButton.png" );
         break;
// Need unit specific go buttons

      case PL_CAST_HEAL:
         base_tex = t_manager.getTexture( "CastHeal.png" );
         break;
      case PL_CAST_LIGHTNING:
         base_tex = t_manager.getTexture( "CastLightning.png" );
         break;
      case PL_CAST_QUAKE:
         base_tex = t_manager.getTexture( "CastQuake.png" );
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

      case MAG_MEDITATE:
      case FAILED_SUMMON:
      case SKIP:
         base_tex = NULL;

   }
   return base_tex;
}

void drawOrder( Order o, int x, int y, int size )
{
   // Base order
   Texture *base_tex = getOrderTexture( o );
   if (NULL == base_tex)
      return;

   Sprite sp_order( *base_tex );
   normalizeTo1x1( &sp_order );
   sp_order.scale( size, size );
   sp_order.setPosition( x, y );

   SFML_GlobalRenderWindow::get()->draw( sp_order );

   // Count
   if (o.count != 1)
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
   if (o.condition != TRUE) {

   }
}

};
