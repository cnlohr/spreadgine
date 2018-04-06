#include <spreadgine.h>
#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <libsurvive/survive.h>
#include <string.h>

void HandleKey( int keycode, int bDown )
{
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

int gargc;
char ** gargv;
SurvivePose phmd;
SurvivePose wm0p;
SurvivePose wm1p;
Spreadgine * gspe;

void my_raw_pose_process(SurviveObject *so, uint32_t timecode, SurvivePose *pose)
{
        survive_default_raw_pose_process(so, timecode, pose);
		if( strcmp( so->codename, "HMD" ) == 0 )
	        memcpy( &phmd , pose, sizeof( phmd ) );
		else if( strcmp( so->codename, "WM0" ) == 0 )
			memcpy( &wm0p, pose, sizeof( *pose ) );
		else if( strcmp( so->codename, "WM1" ) == 0 )
			memcpy( &wm1p, pose, sizeof( *pose ) );
        //printf("%s POSE %0.6f %0.6f %0.6f %0.6f %0.6f %0.6f %0.6f\n", so->codename, pose->Pos[0], pose->Pos[1],       pose->Pos[2], p$
}

void * LibSurviveThread()
{       
        struct SurviveContext *ctx = survive_init(gargc,gargv);

        if (ctx == 0) // implies -help or similiar
                return 0;

        survive_install_pose_fn(ctx, my_raw_pose_process);

        survive_startup(ctx);

        while (survive_poll(ctx) == 0) {
        }
}


void SetupEyes()
{
	double p[3] = { 1, 1, 1} ;
	double eye1[3] = { 1, 1, 1 };
	double eye2[3] = { 1, 1, 1 };
	double at1[3] = { 1, 1, 1 };
	double at2[3] = { 1, 1, 1 };
	double up[3] = { 1, 1, 1 };

	double pin[3] = {  0.0, 0., 0 }; //Left eye
	double pineye1[3] = {  0.030, 0., 0 }; //Left eye
	double pineye2[3] = { -0.030, 0., 0 };
	double pinat1[3] = {  0.130, 0., 1 }; //Left eye
	double pinat2[3] = { -0.130, 0., 1 };

	double pinup[3] = { 0, 1., 0 };

	ApplyPoseToPoint(p, &phmd, pin);
	ApplyPoseToPoint(eye1, &phmd, pineye1);
	ApplyPoseToPoint(eye2, &phmd, pineye2);

	ApplyPoseToPoint(at1, &phmd, pinat1);
	ApplyPoseToPoint(at2, &phmd, pinat2);

	ApplyPoseToPoint(up, &phmd, pinup);
	up[0] -= p[0];
	up[1] -= p[1];
	up[2] -= p[2];

	
	int i;
	for( i = 0; i < 2; i++ )
	{
		double * atb[2] = { &at1[0], &at2[0] };
		double * eyeb[2] = { eye1, eye2 };

#if 1
		float leye[3] = { eyeb[i][0], eyeb[i][1], eyeb[i][2] };
		float lat[3] = { atb[i][0], atb[i][1], atb[i][2] };
		float lup[3] = { up[0], up[1], up[2] };
#else
		float leye[3] = { 1, 1, 1 };
		float lat[3] = { 0, 0, 0};
		float lup[3] = { 0, 0, 1 };
#endif

//		printf( "Eye: %f %f %f\n", leye[0], leye[1], leye[2] );
//		printf( " At: %f %f %f\n", lat[0], lat[1], lat[2] );
//		printf( " Up: %f %f %f\n", lup[0], lup[1], lup[2] );

		//Shift vanishing point, since center-of-display is not center-of-vision.


		tdIdentity( gspe->vpviews[i] );
//		if( i == 0 )
//			tdTranslate( gspe->vpviews[0], .2, 0, 0 ); 
//		else
//			tdTranslate( gspe->vpviews[1], -.2, 0, 0 ); 
		tdLookAt( gspe->vpviews[i], leye, lat, lup );

		SpreadChangeCameaView(gspe, i, gspe->vpviews[i] );
	}


}


int main( int argc, char ** argv )
{
	Spreadgine * e = gspe = SpreadInit( 2160, 1200, "Spread Survive Test", 8889, 2, stderr );

	gargc = argc;
	gargv = argv;

	OGCreateThread( LibSurviveThread, e );


/*
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
*/

	//e->geos[0].render_type = GL_LINES;
	//UpdateSpreadGeometry( &e->geos[0], -1, 0 );

//	SpreadSetupCamera( e, 0, 80, ((float)1080)/((float)1200), .1, 200., "left" );
//	SpreadSetupCamera( e, 1, 80, ((float)1080)/((float)1200), .1, 200., "right" );

	tdMode( tdMODELVIEW );

	int x, y, z;

	int frames = 0, tframes = 0;
	double lastframetime = OGGetAbsoluteTime();
	double TimeSinceStart = 0;
	double Last = OGGetAbsoluteTime();
	while(1)
	{
		double Now = OGGetAbsoluteTime();
		spglClearColor( e, .1, 0.1, 0.1, 1.0 );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );

		spglEnable( e, GL_CULL_FACE );

		SetupEyes();

		int x, y;

        tdMode( tdMODELVIEW );
        tdIdentity( gSMatrix );

		spglLineWidth( e, 4 );

		SpreadApplyShader( &e->shaders[0] );

/*		tdPush();
		tdTranslate( gSMatrix, wm0p.Pos[0], wm0p.Pos[1], wm0p.Pos[2] );
		tdScale( gSMatrix, .1, .1, .1 );
		SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
		tdPop();
*/


		tdPush();
		tdTranslate( gSMatrix, wm0p.Pos[0], wm0p.Pos[1], wm0p.Pos[2] );
		tdScale( gSMatrix, .1, .1, .1 );
		SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
		tdPop();

		tdPush();
		tdTranslate( gSMatrix, wm1p.Pos[0], wm1p.Pos[1], wm1p.Pos[2] );
		tdScale( gSMatrix, .1, .1, .1 );
		SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
		tdPop();

//		printf( "%f %f %f / %f %f %f / %f %f %f\n", wm0p.Pos[0], wm0p.Pos[1], wm0p.Pos[2], wm1p.Pos[0], wm1p.Pos[1], wm1p.Pos[2], phmd.Pos[0], phmd.Pos[1], phmd.Pos[2] );

		tdPush();
		tdScale( gSMatrix, .2, .2, .2 );		//Operates ON f

		float ssf[4] = { TimeSinceStart, 0, 0, 0 };
		int slot = SpreadGetUniformSlot( &e->shaders[0], "timevec");
		if( slot >= 0 )
		{
			//printf( "%f\n", ssf[0] );
			SpreadUniform4f( &e->shaders[0], slot, ssf );
		}


		tdTranslate( gSMatrix, -0., -0., -0. );
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
					tdScale( gSMatrix, .3, .3, .3 );
					SpreadRenderGeometry( &e->geos[0], gSMatrix, 0, -1 ); 
					tdPop();
				}
				tdPop();
			}
			tdPop();
		}

		tdPop();

		spglSwap( e );

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
