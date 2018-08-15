#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spatialloc.h"

static void FinishMoveUp( Spatialloc * r, int w, int tx, int ty )
{
	int lx, ly;

	uint8_t terminal[w];
	int terms = 0;
	memset( terminal, 0, sizeof( terminal ) );
	for( ly = ty-1; ly >= 0; ly-- )
	{
		for( lx = tx; lx < tx + w; lx++ )
		{
			if( terminal[lx-tx] ) continue;

			int belowme;
			if( ly + 1 == r->h )
				belowme = 0;
			else
				belowme = r->downmap[(ly+1)*r->w+lx];
			if( belowme < 0 ) belowme = 0;
			int me = r->downmap[ly*r->w+lx];

			if( me <= 0 ) //We hit another block.
			{
				terms++;
				terminal[lx-tx] = 1;
			}
			else
			{
				r->downmap[ly*r->w+lx] = belowme+1;
			}
		}
		if( terms == w ) break;
	}
}


Spatialloc * SpatCreate( int w, int h )
{
	int x, y, i;
	Spatialloc * ret = malloc( sizeof( Spatialloc ) );
	ret->w = w;
	ret->h = h;
	ret->downmap = malloc( sizeof( ret->downmap[0] ) * w * h );

	i = 0;
	for( y = 0; y < h; y++ )
	for( x = 0; x < w; x++ )
	{
		ret->downmap[i++] = h - y;
	}
	return ret;
}

int SpatMalloc( struct Spatialloc * r, int w, int h, int * x, int * y )
{
	*x = -1;
	*y = -1;

	int run = 0;

	int tx, ty;
	for( ty = 0; ty < r->h; ty++ )
	{
		//Check to see if we even have a hope at mallocing this line.
		//If not, move on.
		run = 0;
		for( tx = 0; tx < r->w; tx++ )
		{
			if( r->downmap[ty*r->w+tx] >= h )
				run++;
			else
				run = 0;

//			printf( "%3d", r->downmap[ty*r->w+tx] );

			if( run == w ) break;
		}
		if( run == w ) break;
//		printf( " - %d\n", run );
	}
//	printf("\n" );
	if( run == w )
	{
		*x = tx - w + 1;
		*y = ty;
	}
	else
	{
		return -1;
	}

	tx = *x;
	int lx, ly;
	//We founda place we can malloc.  Need to update all the tables.
	for( ly = ty; ly < ty + h; ly++ )
	{
		for( lx = tx; lx < tx + w; lx++ )
		{
			r->downmap[ly*r->w+lx] = (lx == tx)?(ty - ly - 1):0;
		}
	}

	FinishMoveUp( r, w, tx, ty );
	return 0;
}


int	RectFindCorner( struct Spatialloc * r, int x, int y, int * cx, int * cy, int * w, int * h )
{
	if( r->downmap[y*r->w+x] > 0 ) return -1;

	printf( "RFC: %d %d  [%d]\n", x, y, r->downmap[y*r->w+x] );
	//March left, until we find a negative number...
	while( r->downmap[y*r->w+x] == 0 && x > 0 ) x--;

	if( y > 0 )
	printf( "MID: %d %d / %d %d\n", x, y, r->downmap[y*r->w+x], r->downmap[(y-1)*r->w+x] );

	//March up until we run into a situation where the number above us is >= our number...
	while( y > 0 && r->downmap[y*r->w+x] < r->downmap[(y-1)*r->w+x] && r->downmap[(y-1)*r->w+x] < 0 ) y--;
	*cx = x;
	*cy = y;
	printf( " CC: %d %d\n", x, y );

	//March down until we run into the bottom, or the number below us is <= us 
	while( (y < r->h-1) && r->downmap[y*r->w+x] > r->downmap[(y+1)*r->w+x] ) y++;
	while( r->downmap[y*r->w+x+1] == 0 && x < r->w-1 ) x++;
	*w = x - *cx + 1;
	*h = y - *cy + 1;

	printf( " EE: %d %d\n", x, y );
	return 0;
}

int SpatFree( struct Spatialloc * r, int x, int y )
{
	int cx, cy;
	int w, h;
	if( RectFindCorner( r, x, y, &cx, &cy, &w, &h ) )
	{
		return -1;
	}

	//Reset all cells, but rely on FinishMoveUp to properly format them.
	//This could be made faster by doing the up-counting here.
	int lx, ly;
	for( ly = cy; ly < cy + h; ly++ )
	for( lx = cx; lx < cx + w; lx++ )
		r->downmap[lx+ly*r->w] = 1;

	FinishMoveUp( r, w, cx, cy+h );
}


void        SpatPrint( struct Spatialloc * r )
{
	int x, y;
	for( y = 0; y < r->h; y++ )
	{
		for( x = 0; x < r->w; x++ )
		{
			int m = r->downmap[x+y*r->w];
			if( m == 0 )
				printf( "\033[42m" );
			else if( m < 0 )
				printf( "\033[41m" );
			else
				printf( "\033[40m" );
			printf( "%3d", m );
		}
		printf( "\033[40m\n" );
	}
	printf( "\n" );
}

