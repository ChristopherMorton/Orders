#ifndef __UNITS_H
#define __UNITS_H

#include "types.h"
#include "orders.h"

#include <string>
#include <deque>

#include <SFML/System/Vector2.hpp>

namespace sf { class Texture; };

namespace sum
{

enum UnitType {
   // Unique
   PLAYER_T,
   MONSTER_T,
   SOLDIER_T,
   WORM_T,
   BIRD_T,
   BUG_T,
   TARGETPRACTICE_T,
   // Ranged Units
   R_HUMAN_ARCHER_T,
   // Melee Units
   M_HUMAN_SWORDSMAN_T,
   // Special
   SUMMONMARKER_T
};

// BASE CLASS

class Unit
{
public:
   int x_grid, y_grid, x_next, y_next;
   float x_real, y_real;
   Direction facing;

   int team; // 0 = player
   int group; // 1 = main group
   UnitType type;

   int alive;

   int aff_poison;
   int aff_confusion;

   bool flying;

   float health, max_health;
   float speed; // 0.0-1.0 = when do moves complete?
   float vision_range;
   float attack_range;

   float radius;

   float progress;
   int done_attack; // One attack per turn!

   int anim_data;

   bool win_condition; // If true, must die to win the level

   Order *order_queue;
   Order this_turn_order;
   int current_order, final_order, max_orders, order_count;
   int current_iteration;
   int active;

   int turnTo( Direction face );

   int prepareBasicOrder( Order &o, bool cond );
   int startBasicOrder( );
   int updateBasicOrder( float dtf, Order o );
   int completeBasicOrder( Order &o );

   bool evaluateConditional( Order_Conditional oc );

   void activate();
   void clearOrders();

   void logOrders();
   virtual int addOrder( Order o );

   virtual int doAttack( Order o ) = 0;

   virtual int takeDamage( float damage, int flags = 0 );

   virtual std::string descriptor();

   virtual int prepareTurn();
   virtual int startTurn();
   virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture() = 0;
   virtual int draw() = 0;

   virtual ~Unit();
};

// GENERIC UNIT CLASSES

enum AI_Movement {
   MV_HOLD_POSITION,
   MV_PATROL,
   MV_PATROL_PATH,
   MV_FOLLOW_ALLY,
   MV_FREE_ROAM
};

enum AI_Aggression {
   AGR_FLEE,
   AGR_PASSIVE,
   AGR_DEFEND_SELF,
   AGR_ATTACK_IN_RANGE,
   AGR_PURSUE_VISIBLE
};

class AIUnit : public Unit
{
private:
   AIUnit(); // disallowed

public:
   // AI movement data
   AI_Movement ai_move_style;
   std::deque< std::pair<sf::Vector2i,Direction> > ai_waypoints;
   int ai_waypoint_next;
   std::deque<Direction> ai_pathing;

   // AI other data
   AI_Aggression ai_aggro;
   float ai_chase_distance;
   float ai_attack_distance;
   Unit* ai_leader;
   bool ai_overridden;
   int ai_counter;
   int aggroed;

   void setAI( AI_Movement move, AI_Aggression aggro, float chase_dis, float attack_dis, Unit* follow = NULL );

   int addWaypoint( int x, int y, Direction d = ALL_DIR );
   void clearWaypoints();
   int reconstructPath( Direction *came_from_grid, int goal_x, int goal_y );
   int aStar( int start_x, int start_y, int goal_x, int goal_y );
   int aiCalculatePathing( bool next = false );
   int aiFollowPathing();

   virtual int ai();

   AIUnit( UnitType t, int x, int y, Direction face, int my_team );

   //virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual int takeDamage( float damage, int flags = 0 );

   virtual std::string descriptor();

   virtual int prepareTurn();
   //virtual int startTurn();
   virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~AIUnit();
};

// SPECIALIZED CLASSES

class Player : public Unit
{
private:
   Player();
   int init( int grid_x, int grid_y, Direction facing );

public:
   static Player* initPlayer( int grid_x, int grid_y, Direction facing );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   virtual int prepareTurn();
   virtual int startTurn();
   virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Player();
};

class SummonMarker : public Unit
{
private:
   SummonMarker();
public:
   float rotation;
   static SummonMarker* get( int grid_x, int grid_y );

   virtual std::string descriptor();

   virtual int addOrder( Order o );
   virtual int doAttack( Order o );
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~SummonMarker();
};

class Monster : public Unit
{
private:
   Monster(); // Disallowed 

public:
   Monster( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   //virtual int startTurn();
   //virtual int completeTurn();
   //virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Monster();

};

class Soldier : public Unit
{
private:
   Soldier(); // Disallowed 

public:
   int stance; // 0 - axe, 1 - spear, 2 - bow

   Soldier( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   virtual int prepareTurn();
   //virtual int startTurn();
   //virtual int completeTurn();
   //virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Soldier();

};

class Worm : public Unit
{
private:
   Worm(); // Disallowed 

public:
   Worm( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   //virtual int startTurn();
   //virtual int completeTurn();
   //virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Worm();

};

class Bird : public Unit
{
private:
   Bird(); // Disallowed 

public:
   Bird( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   virtual int startTurn();
   //virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Bird();

};

class Bug : public Unit
{
private:
   Bug(); // Disallowed 

public:
   float orb_speed;

   Bug( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   //virtual int startTurn();
   //virtual int completeTurn();
   //virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Bug();
};

class TargetPractice : public Unit
{
private:
   TargetPractice();
public:
   TargetPractice( int grid_x, int grid_y, Direction face );

   virtual std::string descriptor();

   virtual int addOrder( Order o );
   virtual int doAttack( Order o );
   virtual sf::Texture* getTexture();
   virtual int draw();
   virtual ~TargetPractice();
};

Unit *genBaseUnit( UnitType t, int grid_x, int grid_y, Direction face );

// OTHER

int initUnits();
bool testUnitCanMove( Unit *u );

};

#endif
