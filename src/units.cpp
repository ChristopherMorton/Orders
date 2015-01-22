#include "units.h"
#include "level.h"
#include "focus.h"
#include "util.h"
#include "effects.h"
#include "animation.h"
#include "clock.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

#include <set>
#include <cmath>

//////////////////////////////////////////////////////////////////////
// Definitions ---

#define DEATH_TIME 400
#define DEATH_FADE_TIME 300

#define PLAYER_MAX_HEALTH 100
#define PLAYER_MAX_ORDERS 500

#define MONSTER_BASE_HEALTH 600
#define MONSTER_BASE_MEMORY 10
#define MONSTER_BASE_VISION 3.5
#define MONSTER_BASE_SPEED 0.3

#define MONSTER_CLAW_DAMAGE 10

#define SOLDIER_BASE_HEALTH 40
#define SOLDIER_BASE_MEMORY 16
#define SOLDIER_BASE_VISION 6.5
#define SOLDIER_BASE_SPEED 0.4

#define SOLDIER_AXE_DAMAGE 35
#define SOLDIER_SPEAR_DAMAGE 30

#define WORM_BASE_HEALTH 40
#define WORM_BASE_MEMORY 14
#define WORM_BASE_VISION 8.5
#define WORM_BASE_SPEED 0.2

#define BIRD_BASE_HEALTH 200
#define BIRD_BASE_MEMORY 16
#define BIRD_BASE_VISION 6.5
#define BIRD_BASE_SPEED 0.1

#define BUG_BASE_HEALTH 100
#define BUG_BASE_MEMORY 14
#define BUG_BASE_VISION 5.5
#define BUG_BASE_SPEED 0.8
#define BUG_ORB_SPEED 3.0

using namespace sf;
using namespace std;

namespace sum
{

int rand_int = 12345;

#define GRID_AT(GRID,X,Y) (GRID[((X) + ((Y) * level_dim_x))])

//////////////////////////////////////////////////////////////////////
// Base Unit ---

int Unit::prepareBasicOrder( Order &o, bool cond_result )
{
   int nest;

   if (o.action <= WAIT) {
      switch (o.action) {

         // MOVEMENT

         case MOVE_BACK:
            if (cond_result == true) {
               // Set new next location
               if (facing == NORTH) { turnTo(SOUTH); facing = NORTH; }
               else if (facing == SOUTH) { turnTo(NORTH); facing = SOUTH; }
               else if (facing == EAST) { turnTo(WEST); facing = EAST; }
               else if (facing == WEST) { turnTo(EAST); facing = WEST; }
            }
         case MOVE_FORWARD:
            if (cond_result == true) {
               // Can we move there? If not, reset *_next values and quit
               if (!canMove( x_next, y_next, x_grid, y_grid )) {
                  turnTo(facing);
                  return 0;
               }

               return 1;
            } else {
               return 0;
            }
         case TURN_NORTH:
            if (cond_result == true)
               turnTo(NORTH);
            return 0;
         case TURN_EAST:
            if (cond_result == true)
               turnTo(EAST);
            return 0;
         case TURN_SOUTH:
            if (cond_result == true)
               turnTo(SOUTH);
            return 0;
         case TURN_WEST:
            if (cond_result == true)
               turnTo(WEST);
            return 0;
         case TURN_NEAREST_ENEMY:
            if (cond_result == true) {
               Unit *u = getEnemy( x_grid, y_grid, attack_range, ALL_DIR, this, SELECT_CLOSEST );
               if (u) {
                  Direction d = getDirection( x_grid, y_grid, u->x_grid, u->y_grid );
                  turnTo(d);
               }
            }
            return 0;

         // ATTACKING

         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (cond_result == true) {
               done_attack = 0;
               return 1;
            } else {
               return 0;
            }

         // CONTROL

         case START_BLOCK:
            if (cond_result == true && (o.iteration < o.count || o.count == -1)) {
               o.iteration++;
               return 0;
            } else {
               o.iteration = 0;
               nest = 1;
               for (int i = current_order + 1; i < order_count; ++i) {
                  if (order_queue[i].action == START_BLOCK)
                     nest++;

                  if (order_queue[i].action == END_BLOCK) {
                     nest--;
                     if (nest == 0) { // Found matching end block - go to it, then skip
                        current_order = i;
                        return 0;
                     }
                  }
               }
               // Should only reach here if the block isn't closed...
               log("Skipped a START_BLOCK with no matching END_BLOCK");   
               current_order = order_count;
               active = 0;
               return 1;
            }
         case END_BLOCK:
            // Find matching start block, and go to it - that's it
            nest = 1;
            for (int i = current_order - 1; i >= 0; --i) {
               if (order_queue[i].action == END_BLOCK)
                  nest++;

               if (order_queue[i].action == START_BLOCK) {
                  nest--;
                  if (nest == 0) { // Found matching start block - go 1 before it, and skip
                     current_order = i-1;
                     return 0;
                  }
               }
            }
            log("Found an END_BLOCK with no matching START_BLOCK - skipping");
            // Ignore it
            return 0;
         case REPEAT:
            if (cond_result == true && (o.iteration < o.count || o.count == -1)) {
               o.iteration++;
               current_order = -1; // Goto 0
               return 0;
            } else {
               o.iteration = 0;
               return 0;
            }

         case WAIT:
         default:
            return 1;
      }

   }
   else return -1;

}

bool testUnitCanMove( Unit *u )
{
   if (NULL == u) return false;

   bool r = canMoveUnit( u->x_next, u->y_next, u->x_grid, u->y_grid, u );
   if (false == r) {
      u->this_turn_order = Order( BUMP );
   }
   return r;
}

int Unit::startBasicOrder( )
{
   if (this_turn_order.action > WAIT) return -1;

   if (this_turn_order.action == MOVE_FORWARD || 
       this_turn_order.action == MOVE_BACK)
   {
      testUnitCanMove( this );
   }

   return 0;
}

int Unit::updateBasicOrder( float dtf, Order o )
{
   if (o.action <= WAIT) { 
      switch(o.action) {
         case MOVE_BACK:
            dtf = -dtf;
         case MOVE_FORWARD:
            if (facing == NORTH)
               y_real -= dtf;
            else if (facing == SOUTH)
               y_real += dtf;
            else if (facing == WEST)
               x_real -= dtf;
            else if (facing == EAST)
               x_real += dtf;
            return 0;

         case TURN_NORTH:
         case TURN_EAST:
         case TURN_SOUTH:
         case TURN_WEST:
         case TURN_NEAREST_ENEMY:
            log("ERROR: update on turn order");
            return -2;
            
         case ATTACK_CLOSEST:
         case ATTACK_FARTHEST:
         case ATTACK_SMALLEST:
         case ATTACK_BIGGEST:
            if (!done_attack) {
               if (progress >= speed) {
                  doAttack( o );
               }
            }
            return 0;

         case START_BLOCK:
         case END_BLOCK:
         case REPEAT:
            log("ERROR: update on control order");
            return -2;

         case WAIT:
         default:
            return 0;
      }
   }
   else return -1;
}

int Unit::completeBasicOrder( Order &o )
{
   o.logSelf();
   if (o.action <= WAIT) {
      switch (o.action) {
         case MOVE_BACK:
         case MOVE_FORWARD:
            moveUnit( this, x_next, y_next );
            x_grid = x_next;
            y_grid = y_next;
            turnTo(facing); 
         default: 
            return 0;
      }
   }
   else return -1;
}

bool Unit::evaluateConditional( Order_Conditional oc )
{
   bool s = true;
   switch (oc) {
      case TRUE:
         return true;
      case ENEMY_NOT_ADJACENT:
         s = false;
      case ENEMY_ADJACENT:
         return s != !(getEnemyAdjacent( x_grid, y_grid, this, SELECT_CLOSEST, false ) != NULL);
      case ENEMY_NOT_AHEAD:
         s = false;
      case ENEMY_AHEAD:
         return s != !(getEnemyLine( x_grid, y_grid, vision_range, facing, this, SELECT_CLOSEST, false ) != NULL);
      case ENEMY_NOT_IN_RANGE:
         s = false;
      case ENEMY_IN_RANGE:
         return s != !(getEnemy( x_grid, y_grid, vision_range, facing, this, SELECT_CLOSEST, false ) != NULL);
      case ALLY_NOT_ADJACENT:
         s = false;
      case ALLY_ADJACENT:
         return s != !(getEnemyAdjacent( x_grid, y_grid, this, SELECT_CLOSEST, true ) != NULL);
      case ALLY_NOT_AHEAD:
         s = false;
      case ALLY_AHEAD:
         return s != !(getEnemyLine( x_grid, y_grid, vision_range, facing, this, SELECT_CLOSEST, true ) != NULL);
      case ALLY_NOT_IN_RANGE:
         s = false;
      case ALLY_IN_RANGE:
         return s != !(getEnemy( x_grid, y_grid, vision_range, facing, this, SELECT_CLOSEST, true ) != NULL);
      default:
         return true;
   }
}

void Unit::activate()
{
   current_order = 0;
   active = 2;

   if (team == 0)
      addEffect( SE_GO_MARKER, 0.3, x_real, y_real, 0, 0.1 );
}

void Unit::clearOrders()
{
   order_count = 0;
   current_order = 0;
   final_order = 0;
   log("Cleared orders");
}

int Unit::turnTo( Direction face )
{
   facing = face;

   if (face == NORTH) {
      x_next = x_grid;
      y_next = y_grid - 1;
   } else if (face == SOUTH) {
      x_next = x_grid;
      y_next = y_grid + 1;
   } else if (face == EAST) {
      x_next = x_grid + 1;
      y_next = y_grid;
   } else if (face == WEST) {
      x_next = x_grid - 1;
      y_next = y_grid;
   } else if (face == ALL_DIR) {
      return -1;
   }

   return 0;
}

Unit::~Unit()
{
   if (order_queue)
      delete order_queue;

}

void Unit::logOrders()
{
   log("Unit printOrders:");
   for( int i = 0; i < order_count; i ++ )
   {
      if (i == current_order)
         log("CURRENT ORDER:");
      if (i == final_order)
         log("FINAL ORDER:");

      order_queue[i].logSelf();
   }
}

int Unit::addOrder( Order o )
{
   if (o.action <= WAIT) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Unit::prepareTurn()
{
   if (alive != 1) return 0;

   if (active == 2) active = 1; // Start an active turn

   this_turn_order = Order( WAIT );

   if (aff_poison) {
      aff_poison--;
      takeDamage( MONSTER_CLAW_DAMAGE );
   }
   if (aff_confusion) {
      aff_confusion--;
      if (aff_confusion %= 2)
         return 0;
   }

   set<int> visited_orders;
   while (active == 1 && current_order != final_order 
         && visited_orders.find( current_order ) == visited_orders.end()) {
      this_turn_order = order_queue[current_order];
      visited_orders.insert( current_order );
      bool decision = evaluateConditional(this_turn_order.condition);
      int r = prepareBasicOrder(order_queue[current_order], decision);
      // if prepareBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else 
         break;
   }

   if (current_order == final_order)
      this_turn_order = Order( WAIT );

   progress = 0;
   return 0;
}

int Unit::startTurn()
{
   if (alive != 1) return 0;

   startBasicOrder();
   return 0;
}

int Unit::update( float dtf )
{
   if (alive < 0) {
      alive += (int) (1000.0 * dtf);
      if (alive >= 0) alive = 0;
   }

   if (alive == 0)
      return 1;

   progress += dtf;
   if (active == 1 && current_order != final_order) {
      Order &o = this_turn_order;
      return updateBasicOrder( dtf, o );
   }
   return 0;
}

// Run completeBasicOrder, update iterations, select next order
int Unit::completeTurn()
{
   if (alive != 1) return 0;

   if (active == 1 && current_order != final_order) {
      int r = completeBasicOrder(this_turn_order);
      Order &o = order_queue[current_order];
      o.iteration++;
      if (o.iteration >= o.count && o.count != -1) { 
         current_order++;
         o.iteration = 0;
      }
      return r;
   }

   return 0;
}

int Unit::takeDamage( float damage, int flags )
{
   health -= damage;
   if (health <= 0) {
      // Dead!
      alive = -( DEATH_TIME + DEATH_FADE_TIME );
      return -1;
   }

   return 0;
}

string Unit::descriptor()
{
   return "BASIC UNIT";
}

//////////////////////////////////////////////////////////////////////
// AIUnit ---

// Private
AIUnit::AIUnit()
{ }

AIUnit::AIUnit( UnitType t, int x, int y, Direction face, int my_team )
{
   if ( t < R_HUMAN_ARCHER_T || t >= SUMMONMARKER_T ) {
      alive = 0;
      return;
   }

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   type = t;
   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;

   turnTo(face);

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 1;
   team = my_team;
   group = 1;

   switch (t) {
      case R_HUMAN_ARCHER_T:
         radius = 0.3;
         health = max_health = 100;
         vision_range = 8.5;
         attack_range = 8.5;
         speed = 0.6;
         ai_move_style = MV_HOLD_POSITION;
         ai_aggro = AGR_ATTACK_IN_RANGE;
         ai_leader = NULL;
         break;
      case M_HUMAN_SWORDSMAN_T:
         radius = 0.3;
         health = max_health = 100;
         vision_range = 5.5;
         attack_range = 1.3;
         speed = 0.5;
         ai_move_style = MV_HOLD_POSITION;
         ai_aggro = AGR_ATTACK_IN_RANGE;
         ai_leader = NULL;
         break;
      default:
         break;
   }

   progress = 0;
   
   win_condition = false;
}

int AIUnit::doAttack( Order o )
{
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   if (type >= R_HUMAN_ARCHER_T && type <= R_HUMAN_ARCHER_T) {
      // Ranged attacker
      target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

      if (target) {
         addProjectile( PR_ARROW, team, x_real, y_real, 3.0, attack_range, target, 0.0, 0.1 );
      }
   }

   if (type >= M_HUMAN_SWORDSMAN_T && type <= M_HUMAN_SWORDSMAN_T) {
      // 1-range Melee attacker

      int t_x = x_grid, t_y = y_grid;
      if (addDirection( facing, t_x, t_y ) != -1)
         target = GRID_AT(unit_grid,t_x,t_y);

      if (target) {
         if (type == M_HUMAN_SWORDSMAN_T)
            target->takeDamage( 35 );
      }
   }

   done_attack = 1;
   return 0;
}

int AIUnit::takeDamage( int damage, int flags )
{
   health -= damage;
   if (health <= 0) {
      // Dead!
      alive = -( DEATH_TIME + DEATH_FADE_TIME );
      return -1;
   }

   if (ai_aggro == AGR_DEFEND_SELF)
      aggroed = 1;

   return 0;
}

// AI calculations
void AIUnit::setAI( AI_Movement move, AI_Aggression aggro, float chase_dis, float attack_dis, Unit* follow )
{
   ai_move_style = move;
   ai_aggro = aggro;

   ai_chase_distance = chase_dis;
   if (ai_chase_distance > vision_range)
      ai_chase_distance = vision_range;

   ai_attack_distance = attack_dis;
   if (ai_attack_distance > vision_range)
      ai_attack_distance = vision_range;

   ai_leader = follow;
}

int AIUnit::addWaypoint( int x, int y )
{
   ai_waypoints.push_back( Vector2i( x, y ) );

   if (ai_waypoint_next == -1)
      ai_waypoint_next = 0;

   return ai_waypoints.size();
}

void AIUnit::clearWaypoints()
{
   ai_waypoints.clear();
   ai_waypoint_next = -1;
}

int costEstimateH( int from_x, int from_y, int goal_x, int goal_y )
{
   int dx = abs(from_x - goal_x),
       dy = abs(from_y - goal_y);

   return dx + dy;
}

int AIUnit::reconstructPath( Direction *came_from_grid, int x, int y )
{
   ai_path.clear();

   Direction d;
   while ((d = came_from_grid[ x + (y * level_dim_x) ]) != ALL_DIR) {
      ai_path.push_front( reverseDirection( d ) );
      addDirection( d, x, y );
   }

   return 0;
}

struct aStarVector {
   int x;
   int y;
   int g_score;
   int f_score;

   aStarVector( int _x, int _y, int _g_score, int _f_score ) :
      x(_x), y(_y),
      g_score(_g_score),
      f_score(_f_score) {}
};
inline bool operator== (const aStarVector& lhs, const aStarVector& rhs)
{
   return (lhs.x == rhs.x && lhs.y == rhs.y);
}
bool asvLess (const aStarVector& lhs, const aStarVector& rhs)
{
   return ((lhs.x + (lhs.y * 1000)) < (rhs.x + (rhs.y * 1000)));
}
inline bool operator< (const aStarVector& lhs, const aStarVector& rhs)
{
   return asvLess( lhs, rhs );
}

int AIUnit::aStar( int start_x, int start_y, int goal_x, int goal_y )
{
   set<aStarVector> closed_set;
   set<aStarVector> open_set;

   Direction *came_from_grid = new Direction[level_dim_x * level_dim_y];
   for (int i = 0; i < level_dim_x * level_dim_y; ++i) came_from_grid[i] = ALL_DIR;

   closed_set.clear();
   open_set.clear();
   open_set.insert( aStarVector( start_x, start_y, 0, costEstimateH( start_x, start_y, goal_x, goal_y )));

   while( !open_set.empty() ) {
      // Manually search here for a lowest f_score
      set<aStarVector>::iterator it_next, it_best;
      int it_f_score = 1000;
      for (it_next = open_set.begin(); it_next != open_set.end(); ++it_next) {
         if ((*it_next).f_score < it_f_score) {
            it_best = it_next;
            it_f_score = (*it_next).f_score;
         }
      }

      aStarVector asv = (*it_best);

      // Goal reached
      if (asv.x == goal_x && asv.y == goal_y)
         return reconstructPath(came_from_grid, goal_x, goal_y);

      open_set.erase( it_best );
      closed_set.insert( asv );
      // Find neighbors in each Direction
      for (Direction d = NORTH; d != ALL_DIR; d = (Direction)((int)d + 1)) {
         int neighbor_x = asv.x,
             neighbor_y = asv.y;
         addDirection( d, neighbor_x, neighbor_y );
         aStarVector neighbor( neighbor_x, neighbor_y, 0, 0 );
         if (find( closed_set.begin(), closed_set.end(), neighbor) != closed_set.end())
            continue; // Already closed this node

         // Node has not been closed yet
         int tentative_g_score = asv.g_score + 1;

         set<aStarVector>::iterator it_open = find( open_set.begin(), open_set.end(), neighbor);
         if (it_open == open_set.end() || (*it_open).g_score > tentative_g_score) {
            came_from_grid[ neighbor.x + (neighbor.y * level_dim_x) ] = reverseDirection( d );
            neighbor.g_score = tentative_g_score;
            neighbor.f_score = neighbor.g_score + costEstimateH( neighbor.x, neighbor.y, goal_x, goal_y );
            if (it_open != open_set.end())
               open_set.erase( it_open );
            open_set.insert( neighbor );
         }
      }
   }

   return -1;
}
/*
    closedset := the empty set    // The set of nodes already evaluated.
    openset := {start}    // The set of tentative nodes to be evaluated, initially containing the start node
    came_from := the empty map    // The map of navigated nodes.
 
    g_score[start] := 0    // Cost from start along best known path.
    // Estimated total cost from start to goal through y.
    f_score[start] := g_score[start] + heuristic_cost_estimate(start, goal)
 
    while openset is not empty
        current := the node in openset having the lowest f_score[] value
        if current = goal
            return reconstruct_path(came_from, goal)
 
        remove current from openset
        add current to closedset
        for each neighbor in neighbor_nodes(current)
            if neighbor in closedset
                continue
            tentative_g_score := g_score[current] + dist_between(current,neighbor)
 
            if neighbor not in openset or tentative_g_score < g_score[neighbor] 
                came_from[neighbor] := current
                g_score[neighbor] := tentative_g_score
                f_score[neighbor] := g_score[neighbor] + heuristic_cost_estimate(neighbor, goal)
                if neighbor not in openset
                    add neighbor to openset
 
    return failure
    */
 
int AIUnit::aiCalculatePath()
{
   if (ai_waypoints.size() == 0) return -1;

   // TODO: select destination based on ai_waypoint_next
   Vector2i destination = ai_waypoints.front();

   aStar( x_grid, y_grid, destination.x, destination.y );

   return 0;
}

int AIUnit::aiFollowPath()
{
   if (ai_path.size() == 0) return -1;

   Direction d = ai_path.front();
   turnTo(d);

   if (canMove( x_next, y_next, x_grid, y_grid )) {
      this_turn_order = Order( MOVE_FORWARD );
      ai_path.pop_front();
   }
   else
      this_turn_order = Order( BUMP );


   return 0;
}

std::string AIUnit::descriptor()
{
   return "Human Archer";
}

int AIUnit::ai()
{
   // First see if their aggro is procced
   if (ai_aggro == AGR_FLEE) {
      Unit* enemy = getEnemy( x_grid, y_grid, vision_range, facing, this, SELECT_CLOSEST, false );
      if (enemy != NULL) {
         Direction d = getDirection( enemy->x_grid, enemy->y_grid, x_grid, y_grid );
         turnTo(d);
         if (canMove( x_next, y_next, x_grid, y_grid ))
            this_turn_order = Order( MOVE_FORWARD );
         aggroed = 1;
         return 0;
      }

      if (aggroed == 1) {
         aiCalculatePath();
      }
      aggroed = 0;
   }

   if (ai_aggro == AGR_PASSIVE)
      aggroed = 0;

   if (ai_aggro == AGR_ATTACK_IN_RANGE || (ai_aggro == AGR_DEFEND_SELF && aggroed == 1)) {
      // Try to attack
      Unit* enemy = getEnemy( x_grid, y_grid, ai_attack_distance, facing, this, SELECT_CLOSEST, false );
      if (enemy != NULL) {
         this_turn_order = Order( ATTACK_CLOSEST );
         done_attack = 0;
         aggroed = 1;
         return 0;
      }

      if (aggroed == 1) {
         aiCalculatePath();
      }
      aggroed = 0;
   }

   if (ai_aggro == AGR_PURSUE_VISIBLE) {
      // Try to attack
      Unit* enemy = getEnemy( x_grid, y_grid, ai_attack_distance, facing, this, SELECT_CLOSEST, false );
      if (enemy != NULL) {
         this_turn_order = Order( ATTACK_CLOSEST );
         done_attack = 0;
         aggroed = 1;
         return 0;
      }

      // Try to chase
      enemy = getEnemy( x_grid, y_grid, ai_chase_distance, facing, this, SELECT_CLOSEST, false );
      if (enemy != NULL)
      {
         Direction d = getDirection( x_grid, y_grid, enemy->x_grid, enemy->y_grid );
         turnTo(d);
         if (canMove( x_next, y_next, x_grid, y_grid ))
            this_turn_order = Order( MOVE_FORWARD );
         aggroed = 1;
         return 0;
      }

      if (aggroed == 1) {
         aiCalculatePath();
      }
      aggroed = 0;
   }


   // Second, if not aggroed, move in the customary way
   if (aggroed == 0) {

      if (ai_move_style == MV_HOLD_POSITION) {
         if (ai_waypoints.size() == 0) return -1;

         Vector2i h_pos = ai_waypoints.front();
         if (h_pos.x == x_grid && h_pos.y == y_grid) {// at position
            this_turn_order = Order( WAIT );
            return 0;
         }
         else
            return aiFollowPath();
      }


      /*
      if (ai_move_style == MV_PATROL_PATH) {
         // TODO: move waypoint in completeTurn
         if (ai_waypoints.size() == 0) return -1;

         Vector2i h_pos = ai_waypoints.front();
         if (h_pos.x == x_grid && h_pos.y == y_grid) {// at position
            this_turn_order = Order( FOLLOW_PATH );
            return 0;
         }
         else
            return aiFollowPath();
      }
      */


      if (ai_move_style == MV_FREE_ROAM) {
         this_turn_order = Order( WAIT );
         return 0;
      }
   }

   return 0;
}

int AIUnit::prepareTurn()
{
   if (alive != 1) return 0;

   if (aff_poison) {
      aff_poison--;
      takeDamage( 10 );
   }
   if (aff_confusion) {
      aff_confusion--;
      if (aff_confusion %= 2)
         return 0;
   }

   if (ai_overridden == true)
      return 0; // Received orders from elsewhere

   this_turn_order = Order( WAIT );
   
   ai();

   return 0;
}

int AIUnit::update( float dtf )
{
   if (alive < 0) {
      alive += (int) (1000.0 * dtf);
      if (alive >= 0) alive = 0;
   }

   if (alive == 0)
      return 1;

   progress += dtf;
   Order &o = this_turn_order;
   return updateBasicOrder( dtf, o );
}

int AIUnit::completeTurn()
{
   if (alive != 1) return 0;

   ai_overridden = false; // reset override

   completeBasicOrder(this_turn_order);

   return 0;
}

sf::Texture* AIUnit::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "HumanArcher.png" );
}

Sprite *sp_human_archer = NULL;

int AIUnit::draw()
{
   if (NULL == sp_human_archer) {
      sp_human_archer = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_human_archer->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_human_archer );
      //sp_human_archer->scale( 0.5, 0.5 );
   }

   sp_human_archer->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_human_archer->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_human_archer );

   return 0;

}

AIUnit::~AIUnit()
{ }

//////////////////////////////////////////////////////////////////////
// Player ---

// Static

Player *thePlayer = NULL;

Animation player_anim_idle;

void initPlayerAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "BugScratchWiggleAnimation.png" );
   player_anim_idle.load( t, 128, 128, 3, 1000 );
}

// Private
Player::Player()
{ }

Player::~Player()
{
   type = PLAYER_T;
}

Player* Player::initPlayer( int grid_x, int grid_y, Direction facing )
{
   if (NULL == thePlayer)
      thePlayer = new Player();
   
   thePlayer->init( grid_x, grid_y, facing );
   return thePlayer;
}

int Player::init( int x, int y, Direction face )
{
   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.4;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = PLAYER_MAX_HEALTH;

   vision_range = 0.5;
   attack_range = 3.5;

   speed = 0.99;

   max_orders = PLAYER_MAX_ORDERS;
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   initPlayerAnimations();

   active = 0;
   team = 0; 
   group = 1;
   progress = 0;
   
   win_condition = false;

   return 0;
}

int Player::addOrder( Order o )
{
   // Player uses a looping queue instead of a list
   if (order_count >= PLAYER_MAX_ORDERS) // No more memory!!
      return -2;

   order_queue[final_order] = o;
   order_count++;
   final_order++;
   if (final_order == PLAYER_MAX_ORDERS)
      final_order = 0;

   return 0;
}

int Player::doAttack( Order o )
{
   // Has no attack
   return 0;
}

int Player::prepareTurn()
{
   if (current_order == final_order) { // Nothing to do
      active = 0;
      return -1;
   }
   else active = 1;

   // Here is where we shout a new command
   Order o = order_queue[current_order];
   if (o.action == PL_DELAY)
      return 0;

   if (o.action < PL_ALERT_ALL)
      broadcastOrder( o );
   else {
      if (startPlayerCommand( o ) == -2)
         order_queue[current_order].action = FAILED_SUMMON;
   }

   progress = 0;
   return 0;
}

int Player::startTurn()
{

   return 0;
}

int Player::completeTurn()
{
   if (current_order == final_order) // Nothing to do
      return -1;

   if (active) { // started the turn doing something
      Order &o = order_queue[current_order];
      if (o.action == PL_DELAY) {
         o.count--;
         if (o.count > 0)
            return 0;
      }

      if (o.action >= PL_ALERT_ALL)
         completePlayerCommand(o);

      do {
         current_order++;
         order_count--;
         if (current_order == PLAYER_MAX_ORDERS)
            current_order = 0; // loop
      } while (order_queue[current_order].action == SKIP);
   }

   return 0;
}

int Player::update( float dtf )
{
   if (alive == 0)
      return 1;

   progress += dtf;
   return 0;
}

sf::Texture* Player::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BugScratch.png" );
}

int Player::draw()
{ 
   Sprite *sp_player = player_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_player) return -1;

   Vector2u dim (player_anim_idle.image_size_x, player_anim_idle.image_size_y);
   sp_player->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_player->setScale( 1.0 / dim.x, 1.0 / dim.y );

   sp_player->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_player->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_player );

   return 0;
}

string Player::descriptor()
{
   return "Sheherezad";
}

//////////////////////////////////////////////////////////////////////
// Monster ---

Animation monster_anim_idle;
Animation monster_anim_move;
Animation monster_anim_attack_start;
Animation monster_anim_attack_end;
Animation monster_anim_death;

void initMonsterAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "MonsterAnimIdle.png" );
   monster_anim_idle.load( t, 128, 128, 8, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "MonsterAnimMove.png" );
   monster_anim_move.load( t, 128, 128, 14, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "MonsterAnimAttackStart.png" );
   monster_anim_attack_start.load( t, 128, 128, 8, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "MonsterAnimAttackEnd.png" );
   monster_anim_attack_end.load( t, 128, 128, 7, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "MonsterAnimDeath.png" );
   monster_anim_death.load( t, 128, 128, 7, DEATH_TIME );
}

// *tors
Monster::Monster()
{
   Monster( -1, -1, SOUTH );
}

Monster::Monster( int x, int y, Direction face )
{
   log("Creating Monster");

   type = MONSTER_T;

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.5;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = MONSTER_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = MONSTER_BASE_VISION * (1.0 + ((float)focus_perception / 25.0));
   attack_range = 1.3;

   speed = MONSTER_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = MONSTER_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;

   win_condition = false;
}

Monster::~Monster()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Monster::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= MONSTER_GUARD && o.action <= MONSTER_BURST)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Monster::doAttack( Order o )
{
   log("Monster doAttack");
   Unit *target = NULL;

   int t_x = x_grid, t_y = y_grid;
   if (addDirection( facing, t_x, t_y ) != -1)
      target = GRID_AT(unit_grid,t_x,t_y);

   if (target) {
      log("Monster attack");
      target->takeDamage( 15 );
      // TODO: Animate
   }

   done_attack = 1;
   return 0;
}

/*
int Monster::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Monster::completeTurn()
{

   return 0;
}

int Monster::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Monster::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "MonsterStatic.png" );
}

int Monster::draw()
{
   // Select sprite
   Sprite *sp_monster = NULL;

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;

   if (alive < 0) {
      // Death animation
      int t = alive + DEATH_TIME + DEATH_FADE_TIME;
      if (t >= DEATH_TIME) t = DEATH_TIME - 1;
      sp_monster = monster_anim_death.getSprite( t );

      int alpha = 255;
      if (alive > -DEATH_FADE_TIME)
         alpha = 255 - ((DEATH_FADE_TIME + alive) * 256 / DEATH_FADE_TIME);
      sp_monster->setColor( Color( 255, 255, 255, alpha ) );
   } else 
   if (this_turn_order.action == MOVE_FORWARD) {
      sp_monster = monster_anim_move.getSprite( (int)(progress * 1000) );
   } else if (this_turn_order.action == MOVE_BACK) {
      sp_monster = monster_anim_move.getSprite( 999 - (int)(progress * 1000) );
   } else if (this_turn_order.action >= ATTACK_CLOSEST && this_turn_order.action <= ATTACK_SMALLEST) {
      if (done_attack) {
         int d_anim = (int)( ((progress - speed) / (1-speed)) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_monster = monster_anim_attack_end.getSprite( d_anim );
      } else {
         int d_anim = (int)( (progress / speed) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_monster = monster_anim_attack_start.getSprite( d_anim );
         sp_monster = monster_anim_attack_start.getSprite( (int)( (progress / speed) * 1000) );
      }
   } else
      sp_monster = monster_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_monster) return -1;

   // Move/scale sprite
   sp_monster->setPosition( x_real, y_real );
   Vector2u dim (monster_anim_idle.image_size_x, monster_anim_idle.image_size_y);
   sp_monster->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_monster->setScale( 1.0 / dim.x, 1.0 / dim.y );

   sp_monster->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_monster );

   return 0;
}

string Monster::descriptor()
{
   return "Allied Monster";
}

//////////////////////////////////////////////////////////////////////
// Soldier ---

Animation soldier_anim_idle;
Animation soldier_anim_idle_axe;
Animation soldier_anim_move_axe;
Animation soldier_anim_attack_start_axe;
Animation soldier_anim_attack_end_axe;
Animation soldier_anim_idle_spear;
Animation soldier_anim_move_spear;
Animation soldier_anim_attack_start_spear;
Animation soldier_anim_attack_end_spear;
Animation soldier_anim_idle_bow;
Animation soldier_anim_move_bow;
Animation soldier_anim_attack_start_bow;
Animation soldier_anim_attack_end_bow;
Animation soldier_anim_death;

void initSoldierAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "SoldierStatic.png" );
   soldier_anim_idle.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimIdleAxe.png" );
   soldier_anim_idle_axe.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimMoveAxe.png" );
   soldier_anim_move_axe.load( t, 128, 128, 16, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackStartAxe.png" );
   soldier_anim_attack_start_axe.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackEndAxe.png" );
   soldier_anim_attack_end_axe.load( t, 128, 128, 9, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimIdleSpear.png" );
   soldier_anim_idle_spear.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimMoveSpear.png" );
   soldier_anim_move_spear.load( t, 128, 128, 16, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackStartSpear.png" );
   soldier_anim_attack_start_spear.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackEndSpear.png" );
   soldier_anim_attack_end_spear.load( t, 128, 128, 9, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimIdleBow.png" );
   soldier_anim_idle_bow.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimMoveBow.png" );
   soldier_anim_move_bow.load( t, 128, 128, 16, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackStartBow.png" );
   soldier_anim_attack_start_bow.load( t, 128, 128, 11, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimAttackEndBow.png" );
   soldier_anim_attack_end_bow.load( t, 128, 128, 11, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "SoldierAnimDeath.png" );
   soldier_anim_death.load( t, 128, 128, 9, DEATH_TIME );
}

// *tors
Soldier::Soldier()
{
   Soldier( -1, -1, SOUTH );
}

Soldier::Soldier( int x, int y, Direction face )
{
   log("Creating Soldier");

   type = SOLDIER_T;

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = SOLDIER_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   stance = 0;

   vision_range = SOLDIER_BASE_VISION * (1.0 + ((float)focus_perception / 25.0));
   attack_range = 1.3;

   speed = SOLDIER_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = SOLDIER_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;

   win_condition = false;
}

Soldier::~Soldier()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Soldier::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= SOLDIER_SWITCH_AXE && o.action <= SOLDIER_SWITCH_BOW)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Soldier::doAttack( Order o )
{
   Unit *target = NULL;
   int selector = SELECT_CLOSEST;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   if (stance == 0) {
      int t_x = x_grid, t_y = y_grid;
      if (addDirection( facing, t_x, t_y ) != -1)
         target = GRID_AT(unit_grid,t_x,t_y);

      if (target) {
         log("Soldier axe attack");
         target->takeDamage( SOLDIER_AXE_DAMAGE );
         // TODO: Animate
      }
   }
   else if (stance == 1) {
      int min_x = x_grid - 2, max_x = x_grid + 2, min_y = y_grid - 2, max_y = y_grid + 2;
      if (facing == NORTH) max_y = y_grid - 1;
      else if (facing == SOUTH) min_y = y_grid + 1;
      else if (facing == WEST) max_x = x_grid - 1;
      else if (facing == EAST) min_x = x_grid + 1;

      target = getEnemyBox( x_grid, y_grid, min_x, max_x, min_y, max_y, attack_range, this, selector );

      if (target) {
         log("Soldier spear attack");
         // Get spear trajectory
         int t_x = target->x_grid, t_y = target->y_grid;
         if (t_x == x_grid + 1) {
            if (t_y == y_grid + 1) {
               t_x = x_grid + 2;
               t_y = y_grid + 2;
            } else if (t_y == y_grid - 1) {
               t_x = x_grid + 2;
               t_y = y_grid - 2;
            } else if (t_y == y_grid)
               t_x = x_grid + 2;
         } else if (t_x == x_grid - 1) {
            if (t_y == y_grid + 1) {
               t_x = x_grid - 2;
               t_y = y_grid + 2;
            } else if (t_y == y_grid - 1) {
               t_x = x_grid - 2;
               t_y = y_grid - 2;
            } else if (t_y == y_grid)
               t_x = x_grid - 2;
         } else if (t_x == x_grid) {
            if (t_y == y_grid - 1) t_y = y_grid - 2;
            else if (t_y == y_grid + 1) t_y = y_grid + 2;
         }
         // Get inner target
         int t_x2 = t_x, t_y2 = t_y;
         if (t_x2 == x_grid + 2) t_x2 = x_grid + 1;
         if (t_x2 == x_grid - 2) t_x2 = x_grid - 1;
         if (t_y2 == y_grid + 2) t_y2 = y_grid + 1;
         if (t_y2 == y_grid - 2) t_y2 = y_grid - 1;

         // Damage targets
         Unit *t1 = GRID_AT(unit_grid,t_x,t_y);
         Unit *t2 = GRID_AT(unit_grid,t_x2,t_y2);
         if (NULL != t1)
            t1->takeDamage( SOLDIER_SPEAR_DAMAGE );
         if (NULL != t2)
            t2->takeDamage( SOLDIER_SPEAR_DAMAGE );

         // Animate
         float rot = atan( (t_y - y_grid) / (t_x - x_grid + 0.0001) ) * 180 / 3.1415926;
         addEffect( SE_SPEAR_ANIM, 0.15, x_real, y_real, rot, 0.15 );
      }

   }
   else if (stance == 2) {
      target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

      if (target) {
         log("Soldier bow attack");
         addProjectile( PR_ARROW, team, x_real, y_real, 3.0, attack_range, target, 0.0, 0.1 );
      }
   }

   done_attack = 1;
   return 0;
}

int Soldier::prepareTurn()
{
   if (alive != 1) return 0;

   if (active == 2) active = 1; // Start an active turn

   this_turn_order = Order( WAIT );

   if (aff_poison) {
      aff_poison--;
      takeDamage( MONSTER_CLAW_DAMAGE );
   }
   if (aff_confusion) {
      aff_confusion--;
      if (aff_confusion %= 2)
         return 0;
   }

   set<int> visited_orders;
   while (active == 1 && current_order != final_order 
         && visited_orders.find( current_order ) == visited_orders.end()) {
      this_turn_order = order_queue[current_order];
      visited_orders.insert( current_order );
      bool decision = evaluateConditional(this_turn_order.condition);
      // SOLDIER UNIQUE CODE
      int r;
      if (this_turn_order.action <= SKIP)
         r = prepareBasicOrder(order_queue[current_order], decision);
      else if (this_turn_order.action == SOLDIER_SWITCH_AXE) {
         r = 0;
         stance = 0;
         attack_range = 1.3;
      } else if (this_turn_order.action == SOLDIER_SWITCH_SPEAR) {
         r = 0;
         stance = 1;
         attack_range = 3.0;
      } else if (this_turn_order.action == SOLDIER_SWITCH_BOW) {
         r = 0;
         stance = 2;
         attack_range = vision_range;
      }
      // END
      // if prepareBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else 
         break;
   }

   if (current_order == final_order)
      this_turn_order = Order( WAIT );

   progress = 0;
   return 0;
}
/*
int Soldier::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Soldier::completeTurn()
{

   return 0;
}

int Soldier::update( float dtf )
{
   if (alive < 0) {
      alive += (int) (1000.0 * dtf);
      if (alive >= 0) alive = 0;
   }

   if (alive == 0)
      return 1;

   progress += dtf;
   if (active == 1 && current_order != final_order) {
      Order &o = this_turn_order;
      return updateBasicOrder( dtf, o );
   }
   return 0;
}
*/

sf::Texture* Soldier::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "SoldierStatic.png" );
}

int Soldier::draw()
{
   // Select sprite
   Sprite *sp_soldier = NULL;

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;

   if (alive < 0) {
      // Death animation
      int t = alive + DEATH_TIME + DEATH_FADE_TIME;
      if (t >= DEATH_TIME) t = DEATH_TIME - 1;
      sp_soldier = soldier_anim_death.getSprite( t );

      int alpha = 255;
      if (alive > -DEATH_FADE_TIME)
         alpha = 255 - ((DEATH_FADE_TIME + alive) * 256 / DEATH_FADE_TIME);
      sp_soldier->setColor( Color( 255, 255, 255, alpha ) );
   } else if (this_turn_order.action == MOVE_FORWARD) {
      if (stance == 0) sp_soldier = soldier_anim_move_axe.getSprite( (int)(progress * 1000) );
      if (stance == 1) sp_soldier = soldier_anim_move_spear.getSprite( (int)(progress * 1000) );
      if (stance == 2) sp_soldier = soldier_anim_move_bow.getSprite( (int)(progress * 1000) );
   } else if (this_turn_order.action == MOVE_BACK) {
      if (stance == 0) sp_soldier = soldier_anim_move_axe.getSprite( 999 - (int)(progress * 1000) );
      if (stance == 1) sp_soldier = soldier_anim_move_spear.getSprite( 999 - (int)(progress * 1000) );
      if (stance == 2) sp_soldier = soldier_anim_move_bow.getSprite( 999 - (int)(progress * 1000) );
   } else if (this_turn_order.action >= ATTACK_CLOSEST && this_turn_order.action <= ATTACK_SMALLEST) {
      if (done_attack) {
         int d_anim = (int)( ((progress - speed) / (1-speed)) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         if (stance == 0) sp_soldier = soldier_anim_attack_end_axe.getSprite( d_anim );
         if (stance == 1) sp_soldier = soldier_anim_attack_end_spear.getSprite( d_anim );
         if (stance == 2) sp_soldier = soldier_anim_attack_end_bow.getSprite( d_anim );
      } else {
         int d_anim = (int)( (progress / speed) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         if (stance == 0) sp_soldier = soldier_anim_attack_start_axe.getSprite( d_anim );
         if (stance == 1) sp_soldier = soldier_anim_attack_start_spear.getSprite( d_anim );
         if (stance == 2) sp_soldier = soldier_anim_attack_start_bow.getSprite( d_anim );
      }
   } else {
      if (stance == 0) sp_soldier = soldier_anim_idle_axe.getSprite( (int)(progress * 1000) );
      if (stance == 1) sp_soldier = soldier_anim_idle_spear.getSprite( (int)(progress * 1000) );
      if (stance == 2) sp_soldier = soldier_anim_idle_bow.getSprite( (int)(progress * 1000) );
   }
   if (NULL == sp_soldier) return -1;

   // Move/scale sprite
   sp_soldier->setPosition( x_real, y_real );
   Vector2u dim (soldier_anim_idle.image_size_x, soldier_anim_idle.image_size_y);
   sp_soldier->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_soldier->setScale( 0.8 / dim.x, 0.8 / dim.y );

   sp_soldier->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_soldier );

   return 0;
}

string Soldier::descriptor()
{
   return "Allied Soldier";
}


//////////////////////////////////////////////////////////////////////
// Worm ---

Animation worm_anim_idle;
Animation worm_anim_move;
Animation worm_anim_attack_start;
Animation worm_anim_attack_end;
Animation worm_anim_death;

void initWormAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "WormAnimIdle.png" );
   worm_anim_idle.load( t, 128, 128, 8, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "WormAnimMove.png" );
   worm_anim_move.load( t, 128, 128, 11, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "WormAnimAttackStart.png" );
   worm_anim_attack_start.load( t, 128, 128, 8, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "WormAnimAttackEnd.png" );
   worm_anim_attack_end.load( t, 128, 128, 8, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "WormAnimDeath.png" );
   worm_anim_death.load( t, 128, 128, 8, DEATH_TIME );
}

// *tors
Worm::Worm()
{
   Worm( -1, -1, SOUTH );
}

Worm::Worm( int x, int y, Direction face )
{
   log("Creating Worm");

   type = WORM_T;

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = WORM_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = WORM_BASE_VISION * (1.0 + ((float)focus_perception / 25.0));
   attack_range = 1.3;

   speed = WORM_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = WORM_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;
   
   win_condition = false;
}

Worm::~Worm()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Worm::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= WORM_SPRINT && o.action <= WORM_TRAIL_END)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Worm::doAttack( Order o )
{
   log("Worm doAttack");
   Unit *target = NULL;

   int t_x = x_grid, t_y = y_grid;
   if (addDirection( facing, t_x, t_y ) != -1)
      target = GRID_AT(unit_grid,t_x,t_y);

   if (target) {
      log("Worm attack");
      target->takeDamage( 10 );
      target->aff_poison += 20;
      // TODO: Animate
   }

   done_attack = 1;
   return 0;
}

/*
int Worm::prepareTurn()
{
   if (alive != 1) return 0;

   if (active == 2) active = 1; // Start an active turn

   this_turn_order = Order( WAIT );

   while (active == 1 && 
          current_order != final_order) {
      this_turn_order = order_queue[current_order];
      bool decision = evaluateConditional(this_turn_order.condition);
      int r = prepareBasicOrder(this_turn_order, decision);
      // if prepareBasicOrder returns 0, it's a 0-length instruction (e.g. turn)
      if (r == 0) {
         current_order++;
         continue;
      }
      else 
         break;
   }

   if (current_order == final_order)
      this_turn_order = Order( WAIT );

   progress = 0;
   return 0;
}

int Worm::startTurn()
{
   if (alive != 1) return 0;

   startBasicOrder();
   return 0;
}

int Worm::completeTurn()
{
   if (alive == 0) return 0;

   if (active == 1 && current_order != final_order) {
      int r = completeBasicOrder(this_turn_order);
      Order &o = order_queue[current_order];
      o.iteration++;
      if (o.iteration >= o.count && o.count != -1) { 
         current_order++;
         o.iteration = 0;
      }
      return r;
   }

   return 0;
}

int Worm::update( float dtf )
{
   if (alive < 0) {
      alive += (int) (1000 + dtf);
      if (alive >= 0) alive = 0;
   }

   if (alive == 0)
      return 1;

   progress += dtf;
   if (active == 1 && current_order != final_order) {
      Order &o = this_turn_order;
      return updateBasicOrder( dtf, o );
   }
   return 0;
}
*/

sf::Texture* Worm::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "WormStatic.png" );
}

int Worm::draw()
{
   // Select sprite
   Sprite *sp_worm = NULL;
   if (alive < 0) {
      // Death animation
      int t = alive + DEATH_TIME + DEATH_FADE_TIME;
      if (t >= DEATH_TIME) t = DEATH_TIME - 1;
      sp_worm = worm_anim_death.getSprite( t );

      int alpha = 255;
      if (alive > -DEATH_FADE_TIME)
         alpha = 255 - ((DEATH_FADE_TIME + alive) * 256 / DEATH_FADE_TIME);
      sp_worm->setColor( Color( 255, 255, 255, alpha ) );
   } else if (this_turn_order.action == MOVE_FORWARD) {
      sp_worm = worm_anim_move.getSprite( (int)(progress * 1000) );
   } else if (this_turn_order.action == MOVE_BACK) {
      // TODO: Worm should have different retreat animation
      sp_worm = worm_anim_move.getSprite( 999 - (int)(progress * 1000) );
   } else if (this_turn_order.action >= ATTACK_CLOSEST && this_turn_order.action <= ATTACK_SMALLEST) {
      if (done_attack) {
         int d_anim = (int)( ((progress - speed) / (1-speed)) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_worm = worm_anim_attack_end.getSprite( d_anim );
      } else {
         int d_anim = (int)( (progress / speed) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_worm = worm_anim_attack_start.getSprite( d_anim );
      }
   } else
      sp_worm = worm_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_worm) return -1;

   // Move/scale sprite
   Vector2u dim (worm_anim_idle.image_size_x, worm_anim_idle.image_size_y);
   sp_worm->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_worm->setScale( 0.4 / dim.x, 0.4 / dim.y );

   sp_worm->setPosition( x_real, y_real );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_worm->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_worm );

   return 0;
}

string Worm::descriptor()
{
   return "Allied Worm";
}

//////////////////////////////////////////////////////////////////////
// Bird ---

Animation bird_anim_idle1;
Animation bird_anim_idle2;
Animation bird_anim_idle3;
Animation bird_anim_move;
Animation bird_anim_attack_start;
Animation bird_anim_attack_end;
Animation bird_anim_death;

void initBirdAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_idle1.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_idle2.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_idle3.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdAnimMove.png" );
   bird_anim_move.load( t, 128, 128, 10, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_attack_start.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_attack_end.load( t, 128, 128, 1, 1000 );

   t = SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
   bird_anim_death.load( t, 128, 128, 1, DEATH_TIME );
}

// *tors
Bird::Bird()
{
   Bird( -1, -1, SOUTH );
}

Bird::Bird( int x, int y, Direction face )
{
   log("Creating Bird");

   type = BIRD_T;

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.2;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = BIRD_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = BIRD_BASE_VISION * (1.0 + ((float)focus_perception / 25.0));
   attack_range = 1.3;

   speed = BIRD_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = BIRD_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;
   anim_data = 0;
   
   win_condition = false;
}

Bird::~Bird()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Bird::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= BIRD_CMD_SHOUT && o.action <= BIRD_FLY)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Bird::doAttack( Order o )
{
   log("Bird doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Bird attack");
   }

   done_attack = 1;
   return 0;
}

int Bird::startTurn()
{
   if (alive != 1) return 0;

   startBasicOrder();

   if (this_turn_order.action == WAIT) {
      anim_data = (anim_data + 1) % 4;
      if (rand_int % 7 == 0)
         anim_data = -1;
   }

   return 0;
}

/*
int Bird::completeTurn()
{

   return 0;
}
*/

int Bird::update( float dtf )
{
   if (alive < 0) {
      alive += (int) (1000.0 * dtf);
      if (alive >= 0) alive = 0;
   }

   if (alive == 0)
      return 1;

   progress += dtf;
   if (active == 1 && current_order != final_order) {
      Order &o = this_turn_order;
      // Bird Specific
      if (o.action == MOVE_FORWARD || o.action == MOVE_BACK) {
         float d_real = 0.0, d_offset = 0.0;
         if (progress < .1) return 0;
         if (progress >= .1 && progress < .3) {
            d_real = (progress - .1) * 1.5; // 0->0.3
            d_offset = (progress - .1) * -0.5; // 0->-0.1
         } else if (progress > .3 && progress <= .4) {
            d_real = 0.3;
            d_offset = -0.1;
         } else if (progress >= .4 && progress < .6) {
            d_real = (progress - .25) * 2; // 0.3->0.7
            d_offset = (progress - .5); // -0.1->0.1
         } else if (progress > .6 && progress <= .7) {
            d_real = 0.7;
            d_offset = 0.1;
         } else if (progress >= .7 && progress < .9) {
            d_real = (progress - .2333) * 1.5; // 0.7->1.0
            d_offset = (progress - .9) * -0.5; // 0.1->0
         } else if (progress >= .9) {
            d_real = 1.0;
            d_offset = 0.0;
         }

         if (o.action == MOVE_BACK) d_real = -d_real;
         if (facing == NORTH) {
            y_real = y_grid + 0.5 - d_real;
            x_real = x_grid + 0.5 - d_offset;
         } else if (facing == SOUTH) {
            y_real = y_grid + 0.5 + d_real;
            x_real = x_grid + 0.5 + d_offset;
         } else if (facing == WEST) {
            x_real = x_grid + 0.5 - d_real;
            y_real = y_grid + 0.5 - d_offset;
         } else if (facing == EAST) {
            x_real = x_grid + 0.5 + d_real;
            y_real = y_grid + 0.5 + d_offset;
         }

         return 0;
      }
      // End
      return updateBasicOrder( dtf, o );
   }
   return 0;
}

sf::Texture* Bird::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BirdStatic.png" );
}

Sprite *sp_bird = NULL;

int Bird::draw()
{
   // Select sprite
   Sprite *sp_bird = NULL;

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;

   if (alive < 0) {
      // Death animation
      int t = alive + DEATH_TIME + DEATH_FADE_TIME;
      if (t >= DEATH_TIME) t = DEATH_TIME - 1;
      sp_bird = bird_anim_death.getSprite( t );

      int alpha = 255;
      if (alive > -DEATH_FADE_TIME)
         alpha = 255 - ((DEATH_FADE_TIME + alive) * 256 / DEATH_FADE_TIME);
      sp_bird->setColor( Color( 255, 255, 255, alpha ) );
   } else if (this_turn_order.action == MOVE_FORWARD) {
      sp_bird = bird_anim_move.getSprite( (int)(progress * 1000) );
   } else if (this_turn_order.action == MOVE_BACK) {
      // TODO: Worm should have different retreat animation
      sp_bird = bird_anim_move.getSprite( 999 - (int)(progress * 1000) );
   } else if (this_turn_order.action >= ATTACK_CLOSEST && this_turn_order.action <= ATTACK_SMALLEST) {
      if (done_attack) {
         int d_anim = (int)( ((progress - speed) / (1-speed)) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_bird = bird_anim_attack_end.getSprite( d_anim );
      } else {
         int d_anim = (int)( (progress / speed) * 1000);
         if (d_anim >= 1000) d_anim = 999;
         sp_bird = bird_anim_attack_start.getSprite( d_anim );
      }
   } else {
      if (anim_data == 4) sp_bird = bird_anim_idle3.getSprite( (int)(progress * 1000) );
      else if (anim_data == 2) sp_bird = bird_anim_idle2.getSprite( (int)(progress * 1000) );
      else sp_bird = bird_anim_idle1.getSprite( (int)(progress * 1000) );
   }
   if (NULL == sp_bird) return -1;

   // Move/scale sprite
   sp_bird->setPosition( x_real, y_real );
   Vector2u dim (bird_anim_idle1.image_size_x, bird_anim_idle1.image_size_y);
   sp_bird->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_bird->setScale( 0.6 / dim.x, 0.6 / dim.y );

   sp_bird->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_bird );

   return 0;
}

string Bird::descriptor()
{
   return "Allied Bird";
} 

//////////////////////////////////////////////////////////////////////
// Bug ---

Animation bug_anim_idle;

void initBugAnimations()
{
   Texture *t = SFML_TextureManager::getSingleton().getTexture( "BugScratchWiggleAnimation.png" );
   bug_anim_idle.load( t, 128, 128, 3, 1000 );
}

// *tors
Bug::Bug()
{
   Bug( -1, -1, NORTH );
}

Bug::Bug( int x, int y, Direction face )
{
   log("Creating Bug");

   type = BUG_T;

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   radius = 0.4;

   x_grid = x;
   y_grid = y;
   x_real = x + 0.5;
   y_real = y + 0.5;
   turnTo(face);

   health = max_health = BUG_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   vision_range = BUG_BASE_VISION * (1.0 + ((float)focus_perception / 25.0));

   attack_range = BUG_BASE_VISION;
   orb_speed = BUG_ORB_SPEED * ( 1.0 + ( focus_speed * 0.01) );

   speed = BUG_BASE_SPEED * ( 1.0 - ( focus_speed * 0.02 ) );

   max_orders = BUG_BASE_MEMORY * ( 1.0 + ( focus_memory * 0.08 ) );
   order_queue = new Order[max_orders];
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;
   
   win_condition = false;
}

Bug::~Bug()
{
   if (order_queue)
      delete order_queue;
}

// Virtual methods

int Bug::addOrder( Order o )
{
   if ((o.action <= SKIP) || (o.action >= BUG_CAST_FIREBALL && o.action <= BUG_MEDITATE)) {
      if (order_count >= max_orders) // No more memory
         return -2;

      order_queue[order_count] = o;
      order_count++;
      final_order = order_count;

      return 0;
   } else return -1;
}

int Bug::doAttack( Order o )
{
   log("Bug doAttack");
   int selector = SELECT_CLOSEST;
   Unit *target = NULL;
   switch(o.action) {
      case ATTACK_CLOSEST:
         selector = SELECT_CLOSEST;
         break;
      case ATTACK_FARTHEST:
         selector = SELECT_FARTHEST;
         break;
      case ATTACK_SMALLEST:
         selector = SELECT_SMALLEST;
         break;
      case ATTACK_BIGGEST:
         selector = SELECT_BIGGEST;
         break;
      default:
         log("ERROR: doAttack called on non-attack order");
         return -1;
   }

   target = getEnemy( x_grid, y_grid, attack_range, facing, this, selector );

   if (target) {
      log("Bug pre-generate");
      addProjectile( PR_HOMING_ORB, team, x_real, y_real, orb_speed, attack_range, target, 100 );
   }

   log("Bug finishing attack");

   done_attack = 1;
   return 0;
}

/*
int Bug::startTurn()
{
   if (current_order < order_count) {
      Order o = order_queue[current_order];
      startBasicOrder(o);
   }

   return 0;
}

int Bug::completeTurn()
{

   return 0;
}

int Bug::update( float dtf )
{

   return 0;
}
*/

sf::Texture* Bug::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "BugScratch.png" );
}

int Bug::draw()
{
   // Select sprite
   Sprite *sp_bug = bug_anim_idle.getSprite( (int)(progress * 1000) );
   if (NULL == sp_bug) return -1;

   // Move/scale sprite
   sp_bug->setPosition( x_real, y_real );
   Vector2u dim (bug_anim_idle.image_size_x, bug_anim_idle.image_size_y);
   sp_bug->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   sp_bug->setScale( 0.5 / dim.x, 0.5 / dim.y );

   int rotation;
   if (facing == EAST) rotation = 0;
   if (facing == SOUTH) rotation = 90;
   if (facing == WEST) rotation = 180;
   if (facing == NORTH) rotation = 270;
   sp_bug->setRotation( rotation );

   SFML_GlobalRenderWindow::get()->draw( *sp_bug );

   return 0;
}

string Bug::descriptor()
{
   return "Allied Bug";
}

//////////////////////////////////////////////////////////////////////
// SummonMarker ---

SummonMarker::SummonMarker( )
{
   type = SUMMONMARKER_T;

   x_grid = 0;
   y_grid = 0;
   x_real = x_grid + 0.5;
   y_real = y_grid + 0.5;
   turnTo( SOUTH );

   alive = 0;

   aff_poison = 0;
   aff_confusion = 0;

   health = max_health = 0;

   vision_range = 0;
   attack_range = 0;

   radius = 0.45;

   speed = 0.5;

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 0;
   group = 1;
   progress = 0;
   
   win_condition = false;
}

SummonMarker *theSummonMarker = NULL;

SummonMarker* SummonMarker::get( int x, int y )
{
   if (NULL == theSummonMarker)
      theSummonMarker = new SummonMarker();
   
   theSummonMarker->x_grid = x;
   theSummonMarker->x_real = x + 0.5;

   theSummonMarker->y_grid = y;
   theSummonMarker->y_real = y + 0.5;

   theSummonMarker->progress = 0;
   theSummonMarker->rotation = 0;

   return theSummonMarker;
}

int SummonMarker::addOrder( Order o )
{
   // Does no orders
   return 0;
}

int SummonMarker::doAttack( Order o )
{
   // Does no attacks
   return 0;
}

sf::Texture* SummonMarker::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "SummoningCircle.png" );
}

int SummonMarker::update( float dtf )
{
   progress += dtf;
   rotation += (dtf * 450);
   return 0;
}

Sprite *sp_summon_marker_outside = NULL;
Sprite *sp_summon_marker_inside = NULL;

int SummonMarker::draw()
{
   if (NULL == sp_summon_marker_inside) {
      sp_summon_marker_inside = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_summon_marker_inside->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   }
   if (NULL == sp_summon_marker_outside) {
      sp_summon_marker_outside = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_summon_marker_outside->setOrigin( dim.x / 2.0, dim.y / 2.0 );
   }

   normalizeTo1x1( sp_summon_marker_inside );
   normalizeTo1x1( sp_summon_marker_outside );

   sp_summon_marker_inside->setPosition( x_real, y_real );
   sp_summon_marker_outside->setPosition( x_real, y_real );

   if (progress < 0.6) {
      sp_summon_marker_inside->setRotation( rotation );
      sp_summon_marker_outside->setRotation( -rotation );

      float out_scale, in_scale;
      out_scale = (5.0 - (progress/0.3)) / 5.0;
      in_scale = (0.0 + (progress/0.2)) / 5.0;

      sp_summon_marker_outside->scale( out_scale, out_scale );
      sp_summon_marker_inside->scale( in_scale, in_scale );

      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_outside );
      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_inside );

      sp_summon_marker_outside->scale( -out_scale, -out_scale );
      sp_summon_marker_inside->scale( -in_scale, -in_scale );
   }
   else
   {
      int alpha = (int) ( ((progress - 0.6)/0.4) * 256);
      Color c_yellow( 255, 255, 0, alpha );
      CircleShape cir( 0.25 );
      cir.setFillColor( c_yellow );
      cir.setOrigin( 0.25, 0.25 );
      cir.setPosition( x_real, y_real );
      SFML_GlobalRenderWindow::get()->draw( cir );

      sp_summon_marker_inside->setRotation( 270 );
      sp_summon_marker_outside->setRotation( -270 );

      sp_summon_marker_outside->scale( 0.6, 0.6 );
      sp_summon_marker_inside->scale( 0.6, 0.6 );

      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_outside );
      SFML_GlobalRenderWindow::get()->draw( *sp_summon_marker_inside );

      sp_summon_marker_outside->scale( -0.6, -0.6 );
      sp_summon_marker_inside->scale( -0.6, -0.6 );
   }


   return 0;
}

SummonMarker::~SummonMarker()
{

}

string SummonMarker::descriptor()
{
   return "Summon Marker";
}

//////////////////////////////////////////////////////////////////////
// TargetPractice ---

TargetPractice::TargetPractice( int x, int y, Direction face )
{
   type = TARGETPRACTICE_T;

   x_grid = x;
   y_grid = y;
   x_real = x_grid + 0.5;
   y_real = y_grid + 0.5;
   turnTo( face );

   alive = 1;

   aff_poison = 0;
   aff_confusion = 0;

   health = max_health = 100000;

   vision_range = 0;
   attack_range = 0;

   radius = 0.45;

   speed = 0.5;

   max_orders = 0;
   order_queue = NULL;
   clearOrders();

   this_turn_order = Order( WAIT );

   active = 0;
   team = 99;
   group = 1;
   progress = 0;
   
   win_condition = false;
}

int TargetPractice::addOrder( Order o )
{
   // Does no orders
   return 0;
}

int TargetPractice::doAttack( Order o )
{
   // Does no attacks
   return 0;
}

sf::Texture* TargetPractice::getTexture()
{
   return SFML_TextureManager::getSingleton().getTexture( "TargetPractice.png" );
}

Sprite *sp_targ = NULL;

int TargetPractice::draw()
{
   if (NULL == sp_targ) {
      sp_targ = new Sprite(*getTexture());
      Vector2u dim = getTexture()->getSize();
      sp_targ->setOrigin( dim.x / 2.0, dim.y / 2.0 );
      normalizeTo1x1( sp_targ );
   }

   sp_targ->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *sp_targ );

   return 0;

}

TargetPractice::~TargetPractice()
{

}

string TargetPractice::descriptor()
{
   return "Target";
}

//////////////////////////////////////////////////////////////////////
// The rest

Unit *genBaseUnit( UnitType t, int grid_x, int grid_y, Direction face )
{
   if (t == PLAYER_T) return NULL;
   if (t == MONSTER_T) return new Monster( grid_x, grid_y, face );
   if (t == SOLDIER_T) return new Soldier( grid_x, grid_y, face );
   if (t == WORM_T) return new Worm( grid_x, grid_y, face );
   if (t == BIRD_T) return new Bird( grid_x, grid_y, face );
   if (t == BUG_T) return new Bug( grid_x, grid_y, face );
   if (t == TARGETPRACTICE_T) return new TargetPractice( grid_x, grid_y, face );
   if (t == R_HUMAN_ARCHER_T) return new AIUnit( t, grid_x, grid_y, face, 1 );
   if (t == M_HUMAN_SWORDSMAN_T) return NULL;
   if (t == SUMMONMARKER_T) return NULL;

   return NULL;
}

int initUnits()
{
   initMonsterAnimations();
   initSoldierAnimations();
   initWormAnimations();
   initBirdAnimations();
   initBugAnimations();
   return 0;
}

};
