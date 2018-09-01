#ifndef _SPATIALLOC_H
#define _SPATIALLOC_H

#include <stdint.h>

typedef struct Spatialloc Spatialloc;

struct Spatialloc
{
	int w, h;
	//If positive: # of cells downward from here that are available
	//If zero: allocated cell, but not left-most column.
	//Of negative: left-most column of allocated area.  Monotonically increasing starting at -1 at the top.
	int16_t * downmap;		
};

Spatialloc * SpatCreate( int w, int h );
int          SpatMalloc( struct Spatialloc * r, int w, int h, int * x, int * y );
int	         SpatFindCorner( struct Spatialloc * r, int x, int y, int * cx, int * cy, int * w, int * h );
int          SpatFree( struct Spatialloc * r, int x, int y );
void         SpatPrint( struct Spatialloc * r );
void         SpatDestroy( struct Spatialloc * r );
#endif

