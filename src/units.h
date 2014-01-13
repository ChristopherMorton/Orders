#ifndef __UNITS_H
#define __UNITS_H

#include "types.h"
#include "orders.h"

namespace sf { class Texture; };

namespace sum
{

class Unit
{
public:
   int x_grid, y_grid, x_next, y_next;
   float x_real, y_real;
   Direction facing;

   int team; // 0 = player

   bool alive;

   float health, max_health;
   float speed; // 0.0-1.0 = when do moves complete?
   float attack_range;

   float radius;

   float progress;
   int done_attack; // One attack per turn!

   Order *order_queue;
   int current_order, max_orders, order_count;
   int current_iteration;
   int active;

   int TurnTo( Direction face );

   int startBasicOrder( Order &o, bool cond );
   int updateBasicOrder( float dtf, Order o );
   int completeBasicOrder( Order &o );

   bool evaluateConditional( Order o );

   void activate();
   void clearOrders();

   void logOrders();
   virtual int addOrder( Order o );

   virtual int doAttack( Order o ) = 0;

   virtual int takeDamage( float damage );

   virtual int startTurn();
   virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture() = 0;
   virtual int draw() = 0;

   virtual ~Unit();
};


class Player : public Unit
{
private:
   Player();
   int init( int grid_x, int grid_y, Direction facing );

public:
   static Player* initPlayer( int grid_x, int grid_y, Direction facing );

   virtual int addOrder( Order o );

   virtual int doAttack( Order o );

   virtual int startTurn();
   virtual int completeTurn();
   virtual int update( float dtf );
   virtual sf::Texture* getTexture();
   virtual int draw();

   virtual ~Player();
};


class Hog : public Unit
{

};

class Warrior : public Unit
{

};

class Worm : public Unit
{

};

class Bird : public Unit
{

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

   virtual int addOrder( Order o );
   virtual int doAttack( Order o );
   virtual sf::Texture* getTexture();
   virtual int draw();
   virtual ~TargetPractice();
};

};

#endif
