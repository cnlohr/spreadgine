#include <stdio.h>
#include <stdlib.h>
#include "spatialloc.h"
#include <unistd.h>

int main()
{
	Spatialloc * m = SpatCreate( 20	, 16 );

	//int SpatMalloc( struct Spatialloc * r, int w, int h, int * x, int * y );
	//int SpatFindCorner( struct Spatialloc * r, int x, int y, int * cx, int * cy, int * w, int * h );
	//int SpatFree( struct Spatialloc * r, int x, int y );

	int i;
	for( i = 0; i < 4000; i++ )
	{
		int x, y, w, h;
		w = (rand() % 10) + 1;
		h = (rand() % 10) + 1;
		int r = SpatMalloc(m, w, h, &x, &y );
		printf( "Malloc: %d (%d, %d, %d, %d)\n", r, x, y, w, h );
//		SpatPrint( m );

		x = rand()%m->w;
		y = rand()%m->h;
		r =  SpatFree( m, x, y );
		printf( "Free %d = (%d, %d)\n", r, x, y );
		SpatPrint( m );

		//usleep(10000);
		//printf( "\033[1;1H" );
	}

}


