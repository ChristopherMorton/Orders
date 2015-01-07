#ifndef __UNITS_H
#define __UNITS_H

#include "types.h"
#include "orders.h"

#include <string>

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
   SUMMONMARKER_T,
   // Ranged Units
   R_HUMAN_ARCHER_T,
   // Melee Units
   M_HUMAN_SWORDSMAN_T
};

enum AI_Type {
   STAND_AND_FIRE
};

// BASE CLASS

class Unit
{
public:
   int x_grid, y_grid, x_next, y_next;
   float x_real, y_real;
   Direction facing;

   int team; // 0 = player
   UnitType type;

   int alive;

   int aff_poison;
   int aff_confusion;

   float health, max_health;
   float speed; // 0.0-1.0 = when do moves complete?
   float vision_range;
   float attack_range;

   float radius;

   float progress;
   int done_attack; // One attack per turn!

   Order *order_queue;
   Order this_turn_order;
   int current_order, final_order, max_orders, order_count;
   int current_iteration;
   int active;

   int TurnTo( Direction face );

   int prepareBasicOrder( Order &o, bool cond );
   int startBasicOrder( );
   int updateBasicOrder( float dtf, Order o );
   int completeBasicOrder( Order &o );

   bool evaluateConditional( Order o );

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

class RangedUnit : public Unit
{
private:
   RangedUnit(); // disallowed
   AI_Type ai_type;
public:
   RangedUnit( UnitType t, int x, int y, Direction face, int my_team );

   //virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual std::string descriptor();

   virtual int prepareTurn();
   //virtual int startTurn();
   //virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~RangedUnit();
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
   float rotation;
public:
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

   //virtual int startTurn();
   //virtual int completeTurn();
   //virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Bird();

};

class Bug : public Unit
{
private:
   Bug(); // Disallowed 

   float orb_speed;

public:
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

// OTHER

int initUnits();
bool testUnitCanMove( Unit *u );

};

#endif
