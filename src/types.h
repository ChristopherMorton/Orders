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

#define FLAG_VIS_TREESIGHT_MASK 0xf
#define FLAG_VIS_FLYING 0x10

//////////////////////////////////////////////////////////////////////
// Terrain ---

typedef unsigned char Terrain;
#define TER_NONE 0
   // Cliffs basic (1-12) - 12
#define TER_CLIFF_S 1
#define TER_CLIFF_S_W_EDGE 2
#define TER_CLIFF_S_E_EDGE 3
#define TER_CLIFF_N 4
#define TER_CLIFF_N_E_EDGE 5
#define TER_CLIFF_N_W_EDGE 6
#define TER_CLIFF_E 7
#define TER_CLIFF_E_S_EDGE 8
#define TER_CLIFF_E_N_EDGE 9
#define TER_CLIFF_W 10
#define TER_CLIFF_W_N_EDGE 11
#define TER_CLIFF_W_S_EDGE 12
   // Cliffs corners (13-20) - 8
#define TER_CLIFF_CORNER_SE_90 13
#define TER_CLIFF_CORNER_SW_90 14
#define TER_CLIFF_CORNER_NW_90 15
#define TER_CLIFF_CORNER_NE_90 16
#define TER_CLIFF_CORNER_SE_270 17
#define TER_CLIFF_CORNER_SW_270 18
#define TER_CLIFF_CORNER_NW_270 19
#define TER_CLIFF_CORNER_NE_270 20
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

// More special terrain
   // Dirt Path (224-233) - 10
#define TER_PATH_N_END 224
#define TER_PATH_E_END 225
#define TER_PATH_S_END 226
#define TER_PATH_W_END 227
#define TER_PATH_EW 228
#define TER_PATH_NS 229
#define TER_PATH_NE 230
#define TER_PATH_SE 231
#define TER_PATH_SW 232
#define TER_PATH_NW 233
   // Edge Cliffs (234-245) - 12
#define TER_EDGE_CLIFF_N 234
#define TER_EDGE_CLIFF_S 235
#define TER_EDGE_CLIFF_E 236
#define TER_EDGE_CLIFF_W 237
#define TER_EDGE_CLIFF_CORNER_SE_90 238
#define TER_EDGE_CLIFF_CORNER_SW_90 239
#define TER_EDGE_CLIFF_CORNER_NW_90 240
#define TER_EDGE_CLIFF_CORNER_NE_90 241
#define TER_EDGE_CLIFF_CORNER_SE_270 242
#define TER_EDGE_CLIFF_CORNER_SW_270 243
#define TER_EDGE_CLIFF_CORNER_NW_270 244
#define TER_EDGE_CLIFF_CORNER_NE_270 245
   // Atelier (246-254) - 9
#define TER_ATELIER 246
#define TER_ATELIER_N_EDGE 247
#define TER_ATELIER_E_EDGE 248
#define TER_ATELIER_S_EDGE 249
#define TER_ATELIER_W_EDGE 250
#define TER_ATELIER_CORNER_NE 251
#define TER_ATELIER_CORNER_SE 252
#define TER_ATELIER_CORNER_SW 253
#define TER_ATELIER_CORNER_NW 254

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
