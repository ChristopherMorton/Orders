#ifndef TYPES_H__
#define TYPES_H__

namespace sum
{

typedef enum 
{
   NORTH,
   EAST,
   SOUTH,
   WEST,
   ALL_DIR
} Direction;

typedef enum {
   BASE_TER_GRASS,
   BASE_TER_MOUNTAIN,
   BASE_TER_UNDERGROUND
} BaseTerrain;

typedef unsigned char Terrain;
#define TER_NONE 0
   // Cliffs basic (1-12) - 12
#define CLIFF_SOUTH 1
#define CLIFF_SOUTH_WEST_EDGE 2
#define CLIFF_SOUTH_EAST_EDGE 3
#define CLIFF_NORTH 4
#define CLIFF_NORTH_EAST_EDGE 5
#define CLIFF_NORTH_WEST_EDGE 6
#define CLIFF_EAST 7
#define CLIFF_EAST_SOUTH_EDGE 8
#define CLIFF_EAST_NORTH_EDGE 9
#define CLIFF_WEST 10
#define CLIFF_WEST_NORTH_EDGE 11
#define CLIFF_WEST_SOUTH_EDGE 12
   // Cliffs corners (13-20) - 8
#define CLIFF_CORNER_SOUTHEAST_90 13
#define CLIFF_CORNER_SOUTHWEST_90 14
#define CLIFF_CORNER_NORTHWEST_90 15
#define CLIFF_CORNER_NORTHEAST_90 16
#define CLIFF_CORNER_SOUTHEAST_270 17
#define CLIFF_CORNER_SOUTHWEST_270 18
#define CLIFF_CORNER_NORTHWEST_270 19
#define CLIFF_CORNER_NORTHEAST_270 20
   // Rocks (21-40) - 9
#define TER_ROCK_1 21
#define TER_ROCK_2 22
#define TER_ROCK_3 23
#define TER_ROCK_4 24
#define TER_ROCK_5 25
#define TER_ROCK_6 26
#define TER_ROCK_7 27
#define TER_ROCK_8 28
#define TER_ROCK_LAST 29
   // Trees (41-59) - 9
#define TER_TREE_1 41
#define TER_TREE_2 42
#define TER_TREE_3 43
#define TER_TREE_4 44
#define TER_TREE_5 45
#define TER_TREE_6 46
#define TER_TREE_7 47
#define TER_TREE_8 48
#define TER_TREE_LAST 49
   // Flowers/Grass (41-60) - 0
   // End
#define NUM_TERRAINS 255

typedef enum {
   VIS_NONE,
   VIS_OFFMAP,
   VIS_NEVER_SEEN,
   VIS_SEEN_BEFORE,
   VIS_VISIBLE
} Vision;

};

#endif 
