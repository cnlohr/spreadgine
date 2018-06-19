#include <spreadgine.h>
#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <libsurvive/survive.h>
#include <string.h>
#include <spread_vr.h>

void HandleKey( int keycode, int bDown )
{
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}


int main( int argc, char ** argv )
{
	Spreadgine * e = gspe = SpreadInit( 2160, 1200, "Spread Survive Test", 8889, 2, stderr );

	gargc = argc;
	gargv = argv;

	SpreadSetupVR();

	tdMode( tdMODELVIEW );

	SpreadGeometry * gun = LoadOBJ( gspe, "assets/simple_gun.obj", 1, 0 );

	int x, y, z;

	int frames = 0, tframes = 0;
	double lastframetime = OGGetAbsoluteTime();
	double TimeSinceStart = 0;
	double Last = OGGetAbsoluteTime();
	double TimeOfLastSwap = OGGetAbsoluteTime();
	while(1)
	{
		double Now = OGGetAbsoluteTime();
		spglClearColor( e, .1, 0.1, 0.1, 1.0 );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );

		spglEnable( e, GL_CULL_FACE );

		SpreadSetupEyes();

		int x, y;

        tdMode( tdMODELVIEW );
        tdIdentity( gSMatrix );

		spglLineWidth( e, 4 );

		SpreadApplyShader( e->shaders[0] );
#if 0
		tdPush();
		tdTranslate( gSMatrix, wmp[0].Pos[0], wmp[0].Pos[1], wmp[0].Pos[2] );
		tdScale( gSMatrix, .1, .1, .1 );
		SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
		tdPop();

#endif
		//Draw watchmen
		tdPush();
		tdTranslate( gSMatrix, wmp[0].Pos[0], wmp[0].Pos[1], wmp[0].Pos[2] );
		tdRotateQuat( gSMatrix, wmp[0].Rot[0], wmp[0].Rot[1], wmp[0].Rot[2], wmp[0].Rot[3] );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		tdPop();

		tdPush();
		tdTranslate( gSMatrix, wmp[1].Pos[0], wmp[1].Pos[1], wmp[1].Pos[2] );
		tdRotateQuat( gSMatrix, wmp[1].Rot[0], wmp[1].Rot[1], wmp[1].Rot[2], wmp[1].Rot[3] );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		tdPop();

//		printf( "%f %f %f / %f %f %f / %f %f %f\n", wmp[0].Pos[0], wmp[0].Pos[1], wmp[0].Pos[2], wmp[1].Pos[0], wmp[1].Pos[1], wmp[1].Pos[2], phmd.Pos[0], phmd.Pos[1], phmd.Pos[2] );

		tdPush();
		tdScale( gSMatrix, .2, .2, .2 );		//Operates ON f

		float ssf[4] = { TimeSinceStart, 0, 0, 0 };
		int slot = SpreadGetUniformSlot( e->shaders[0], "timevec");
		if( slot >= 0 )
		{
			//printf( "%f\n", ssf[0] );
			SpreadUniform4f( e->shaders[0], slot, ssf );
		}


		tdTranslate( gSMatrix, -1., -1., -1. );
		for( z = 0; z < 5; z++ )
		{
			tdTranslate( gSMatrix, 0.0, 0, 3 );
			tdPush();
			for( y = 0; y < 5; y++ )
			{
				tdTranslate( gSMatrix, 0.0, 3, 0 );
				tdPush();
				for( x = 0; x < 5; x++ )
				{
					tdTranslate( gSMatrix, 3, 0, 0 );
					//int rstart = ((tframes)*6)%36;
					tdPush();
					//float sm = sin(x*1.2+y*.3+z*.8+tframes*.05);
					//tdScale( gSMatrix, sm, sm, sm );
					//tdRotateEA( gSMatrix, tframes+z*10, tframes*3.+y*10, tframes*2+x*1 );
					//tdScale( gSMatrix, .3, .3, .3 );
					SpreadRenderGeometry( e->geos[0], gSMatrix, 0, -1 ); 
					tdPop();
				}
				tdPop();
			}
			tdPop();
		}

		tdPop();

		glFlush();
		double TWS = OGGetAbsoluteTime();
		TWS = TimeOfLastSwap-TWS + 1.f/90.f - .0001;
		if( TWS > 0 )	usleep(TWS*1000000);
		spglSwap( e );
		TimeOfLastSwap = OGGetAbsoluteTime();

		SpreadCheckShaders( e );
		frames++;
		tframes++;
		TimeSinceStart += (Now-Last);
		if( Now - lastframetime > 1 )
		{
			printf( "FPS: %d\n", frames );
			frames = 0;
			lastframetime++;
		}
		Last = Now;
	}
}
