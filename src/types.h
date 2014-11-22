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

typedef enum {
   TER_NONE,
   TER_TREE1,
   TER_TREE2,
   NUM_TERRAINS
} Terrain;

typedef enum {
   VIS_NONE,
   VIS_OFFMAP,
   VIS_NEVER_SEEN,
   VIS_SEEN_BEFORE,
   VIS_VISIBLE
} Vision;

};

#endif 
