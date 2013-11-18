// included into level.cpp

#define healthPercent() ((float)health/(float)max_health)

struct Unit
{
public:
   int x_grid, y_grid, x_next, y_next;
   float x_real, y_real;
   Direction facing;

   int team; // 0 = player

   int health, max_health;

   virtual int addOrder( Order o ) = 0;

   virtual int completeTurn() = 0;
   virtual int update( int dt ) = 0;
   virtual int draw() = 0;

   virtual ~Unit();
};


struct Tank : public Unit
{

};

struct Warrior : public Unit
{

};

struct Scout : public Unit
{

};

struct Leiutenant : public Unit
{

};

struct Magician : public Unit
{
private:
   Magician(); // Disallowed

public:
   Magician( int grid_x, int grid_y, Direction face );

   virtual ~Magician();


};
