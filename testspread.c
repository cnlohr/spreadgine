#include <spreadgine.h>
#include <spreadgine_util.h>
#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <linmath.h>

void HandleKey( int keycode, int bDown )
{
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

int main()
{
#if defined( MALI ) || defined( RASPI_GPU )
//	Spreadgine * e = SpreadInit( 1920, 1080, "Spread Test", 8888, 2, stderr );
	Spreadgine * e = SpreadInit( 2160, 1200, "Spread Game Survive Test", 8888, 2, stderr );
#else
	Spreadgine * e = SpreadInit( 800, 600, "Spread Test", 8888, 2, stderr );
#endif

	//First: Add a defualt shader
	SpreadShader * shd1 = SpreadLoadShader( e, "shd1", "assets/autobatch.frag", "assets/autobatch.vert", 0 );
	if( !shd1 )
	{
		fprintf( stderr, "Error making shader.\n" );
	}

#define NUMBATCHO 2000

	SpreadGeometry * sixsquare = MakeSquareMesh( e, 2, 1 );
	BatchedSet * batched   = CreateBatchedSet( e, "batchedTri", NUMBATCHO, 65536, GL_TRIANGLES, 2048, 2048, 8 );
	batched->tex_dirty = 0; //Batch whole updates.
	batched->geo_dirty = 0;

	float eye[3] = { .014, 5, 5 };
	float at[3] =  { 0, 0, 0 };
	float up[3] =  { 0, 0, 1 };
	tdLookAt( e->vpviews[0], eye, at,up );
	tdTranslate( e->vpviews[0], -.4, 0, 0 ); //Shift vanishing point
	eye[0] = -.014;
	tdLookAt( e->vpviews[1], eye, at,up );
	tdTranslate( e->vpviews[1], .4, 0, 0 ); //Shift vanishing point

	SpreadChangeCameaView(e, 0, e->vpviews[0] );
	SpreadChangeCameaView(e, 1, e->vpviews[1] );

	tdMode( tdMODELVIEW );
	tdIdentity( gSMatrix );
	tdTranslate( gSMatrix, 0., 0., 0. );
	tdScale( gSMatrix, .1, .1, .1 );		//Operates ON f
	tdTranslate( gSMatrix, 00., 0., 0. );

	int frames = 0, tframes = 0;
	double lastframetime = OGGetAbsoluteTime();

	BatchedObject * objs[NUMBATCHO];
	int i;
	for( i = 0; i < NUMBATCHO; i++ )
	{
		char stname[1024];
		sprintf( stname, "obj%03d",i);
		objs[i] = AllocateBatchedObject( batched, sixsquare, stname );
	}



	while(1)
	{
		double Now = OGGetAbsoluteTime();
		spglClearColor( e, .0, 0.0, 0.0, 1.0 );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );

		spglLineWidth( e, 4 );

		for( i = 0; i < NUMBATCHO; i++ )
		{
			char stname[1024];
			sprintf( stname, "obj%03d",i);

			double euler[3] = { 0, 0, tframes*.1 };
			LinmathQuat q;
			quatfromeuler( q, euler );

			float quat[4] = { q[0], q[1], q[2], q[3] }; 
			float extra[4] = {  ( i % 5 )/5.0, ((i/5)%5)/5.0, 0., 1. };

			UpdateBatchedObjectTransformData( objs[i], 
				FQuad(  (i % 20) * 1 - 5.0, ((i / 20)%20) * 1 - 5.0, (i/400) + sin( (tframes+i) * .01 ) * 2.5, 1),
				quat, extra );
		}

		//Set up the matrix to display the whole set of 
		tdPush();
		tdIdentity( gSMatrix );
		tdScale( gSMatrix, .3, .3, .3 );
		RenderBatchedSet( batched, shd1, gSMatrix );
		tdPop();

		spglSwap( e );
		SpreadCheckShaders( e );
		frames++;
		tframes++;
		if( Now - lastframetime > 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			lastframetime++;
		}
	}
}
