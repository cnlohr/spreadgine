#include <spreadgine.h>
#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>

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
#ifdef MALI
	Spreadgine * e = SpreadInit( 2160, 1200, "Spread Test", 8888, 2, stderr );
#else
	Spreadgine * e = SpreadInit( 960, 640, "Spread Test", 8888, 2, stderr );
#endif

	SpreadGeometry * gun = LoadOBJ( e, "assets/platform.obj", 0, 0 );

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

	//e->geos[0].render_type = GL_LINES;
	//UpdateSpreadGeometry( &e->geos[0], -1, 0 );

	tdMode( tdMODELVIEW );
	tdIdentity( gSMatrix );
	tdTranslate( gSMatrix, 0., 0., 0. );
	tdScale( gSMatrix, .1, .1, .1 );		//Operates ON f
	tdTranslate( gSMatrix, 00., 0., 0. );

	int x, y;

	int frames = 0, tframes = 0;
	double lastframetime = OGGetAbsoluteTime();
	while(1)
	{
		double Now = OGGetAbsoluteTime();
		spglClearColor( e, .0, 0.0, 0.0, 1.0 );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );

		spglLineWidth( e, 4 );

		tdPush();
		//tdScale( gSMatrix, .1, .1, .1 );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		tdPop();


		SpreadApplyShader( e->shaders[0] );

		tdRotateEA( gSMatrix, 0,.2125,1 );		//Operates ON f
		//tdTranslate( modelmatrix, 0, 0, .1 );

		tdPush();
		tdScale( gSMatrix, 20., 20., 20. );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		//SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
		tdPop();


		tdPush();
		tdTranslate( gSMatrix, -30., -30., 0. );
		for( y = 0; y < 5; y++ )
		{
			tdTranslate( gSMatrix, 0.0, 3, 0 );
			tdPush();
			for( x = 0; x < 20; x++ )
			{
				tdTranslate( gSMatrix, 3, 0, 0 );
				//int rstart = ((tframes)*6)%36;
				SpreadRenderGeometry( e->geos[0], gSMatrix, 0, -1 ); 
			}
			tdPop();
		}
		tdPop();

		//usleep(200000);
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
