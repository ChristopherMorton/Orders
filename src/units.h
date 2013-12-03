#ifndef __UNITS_H
#define __UNITS_H

#include "types.h"
#include "orders.h"

namespace sum
{

class Unit
{
public:
   int x_grid, y_grid, x_next, y_next;
   float x_real, y_real;
   Direction facing;

   int team; // 0 = player

   float health, max_health;

   int TurnTo( Direction face );

   virtual int addOrder( Order o ) = 0;

   virtual int completeTurn() = 0;
   virtual int update() = 0;
   virtual int draw() = 0;

   virtual ~Unit();
};


class Tank : public Unit
{

};

class Warrior : public Unit
{

};

class Scout : public Unit
{

};

class Leiutenant : public Unit
{

};

class Magician : public Unit
{
private:
   Magician(); // Disallowed

public:
   Magician( int grid_x, int grid_y, Direction face );

   virtual int addOrder( Order o );

   virtual int completeTurn();
   virtual int update();
   virtual int draw();

   virtual ~Magician();
};

}

#endif
