	#include <spreadgine.h>
#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <libsurvive/survive.h>
#include <string.h>
#include <math.h>

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
SurviveObject * HMD;
SurviveObject * WM0;
SurviveObject * WM1;
/*
	int32_t buttonmask;
	int16_t axis1;

	int16_t axis2;
	int16_t axis3;
	int8_t charge;
	int8_t charging : 1;
	int8_t ison : 1;
	int8_t additional_flags : 6;
*/

void my_raw_pose_process(SurviveObject *so, uint32_t timecode, SurvivePose *pose)
{
        survive_default_raw_pose_process(so, timecode, pose);
		if( strcmp( so->codename, "HMD" ) == 0 )
		{
	        memcpy( &phmd , pose, sizeof( phmd ) );
			HMD = so;
		}
		else if( strcmp( so->codename, "WM0" ) == 0 )
		{
			memcpy( &wm0p, pose, sizeof( *pose ) );
			WM0 = so;
			
		}
		else if( strcmp( so->codename, "WM1" ) == 0 )
		{
			memcpy( &wm1p, pose, sizeof( *pose ) );
			WM1 = so;
		}
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

float diopter = 0.03;
float disappearing = 0.08;
float fovie = 75;
float eyez = 0;
void SetupEyes()
{
	double p[3] = { 1, 1, 1} ;
	double eye1[3] = { 1, 1, 1 };
	double eye2[3] = { 1, 1, 1 };
	double at1[3] = { 1, 1, 1 };
	double at2[3] = { 1, 1, 1 };
	double up[3] = { 1, 1, 1 };

	double pin[3] = {  0.0, 0., 0 }; //Left eye
	double pineye1[3] = {  diopter, 0., eyez }; //Left eye
	double pineye2[3] = { -diopter, 0., eyez };
	double pinat1[3] = {  disappearing, 0., 1+eyez }; //Left eye
	double pinat2[3] = { -disappearing, 0., 1+eyez };

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

		float leye[3] = { eyeb[i][0], eyeb[i][1], eyeb[i][2] };
		float lat[3] = { atb[i][0], atb[i][1], atb[i][2] };
		float lup[3] = { up[0], up[1], up[2] };

		tdIdentity( gspe->vpviews[i] );
		tdLookAt( gspe->vpviews[i], leye, lat, lup );
		SpreadChangeCameaView(gspe, i, gspe->vpviews[i] );
	}


}

void HandleControllerInput()
{
	if( WM0 )
	{
		static int vsmode;
		static double lasta1, lasta2;
		static int last_bm;
		int bm = WM0->buttonmask;
		if( !(last_bm & 1 ) && (bm &1) )
		{
			vsmode = (vsmode+1)%4;
		}
		if( vsmode == 0 ) 
			spglClearColor( gspe, .1, 0.2, 0.1, 1.0 );
		else if( vsmode == 1)
			spglClearColor( gspe, .2, 0.1, 0.1, 1.0 );
		else if( vsmode == 2 )
			spglClearColor( gspe, .1, 0.1, 0.2, 1.0 );
		else
			spglClearColor( gspe, .2, 0.2, 0.2, 1.0 );
			


		if( !(last_bm & 2 ) && (bm &2) )
		{
			lasta1 = WM0->axis1;
			lasta2 = WM0->axis2;
		}
		double x = WM0->axis2/32767.;
		double y = WM0->axis3/32767.;
		float last_ang = atan2( lasta1, lasta2 );
		float cur_ang = atan2( x, y );
		float delta = cur_ang - last_ang;
		if( delta > 3.14159*2 ) delta -= 3.14159*2;
		if( delta <-3.14159*2 ) delta += 3.14159*2;
		float rang = sqrt(WM0->axis1*WM0->axis1 + WM0->axis2*WM0->axis2);

		if( rang > 10000 && (bm &2) && (last_bm & 2 ) )
		{
			if( vsmode == 0 )
			{
				diopter += delta/100;
			}
			else if( vsmode == 1 )
			{
				disappearing+= delta/100;
			}
			else if( vsmode == 2 )
			{
				fovie += delta;
				SpreadSetupCamera( gspe, 0, fovie, (float)1080/1200, .01, 1000, "CAM0" );
				SpreadSetupCamera( gspe, 1, fovie, (float)1080/1200, .01, 1000, "CAM1" );
			}
			else
			{
				eyez += delta/10.0;
			}
			printf( "WM0: %6.3f %6.3f %6.3f %d %d %f %f %d %f %f %f %f\n", last_ang, cur_ang, delta, WM0->buttonmask, WM0->axis1, x, y, WM0->charge, disappearing, diopter, fovie, eyez );
		}
		last_bm = bm;
		lasta1 = x;
		lasta2 = y;
	}
}

int main( int argc, char ** argv )
{
	Spreadgine * e = gspe = SpreadInit( 2160, 1200, "Spread Game Survive Test", 8888, 2, stderr );

	gargc = argc;
	gargv = argv;

	OGCreateThread( LibSurviveThread, e );

	tdMode( tdMODELVIEW );

	SpreadGeometry * gun = LoadOBJ( gspe, "assets/simple_gun.obj", 1, 1 );
	SpreadGeometry * platform = LoadOBJ( gspe, "assets/platform.obj", 1, 1 );
	e->geos[0]->render_type = GL_LINES;
	UpdateSpreadGeometry( e->geos[0], -1, 0 );
	SpreadSetupCamera( e, 0, fovie, (float)1080/1200, .01, 1000, "CAM0" );
	SpreadSetupCamera( e, 1, fovie, (float)1080/1200, .01, 1000, "CAM1" );


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

/*
	int32_t buttonmask;
	int16_t axis1;
	int16_t axis2;
	int16_t axis3;
	int8_t charge;
	int8_t charging : 1;
	int8_t ison : 1;
	int8_t additional_flags : 6;
*/
		HandleControllerInput();


		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );

		spglEnable( e, GL_CULL_FACE );

		SetupEyes();

		int x, y;

        tdMode( tdMODELVIEW );
        tdIdentity( gSMatrix );

		spglLineWidth( e, 4 );

		SpreadApplyShader( e->shaders[0] );


		tdPush();

		//Draw watchmen
		tdPush();
		tdTranslate( gSMatrix, wm0p.Pos[0], wm0p.Pos[1], wm0p.Pos[2] );
		tdRotateQuat( gSMatrix, wm0p.Rot[0], wm0p.Rot[1], wm0p.Rot[2], wm0p.Rot[3] );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		tdPop();

		tdPush();
		tdTranslate( gSMatrix, wm1p.Pos[0], wm1p.Pos[1], wm1p.Pos[2] );
		tdRotateQuat( gSMatrix, wm1p.Rot[0], wm1p.Rot[1], wm1p.Rot[2], wm1p.Rot[3] );
		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		tdPop();

		SpreadRenderGeometry( platform, gSMatrix, 0, -1 ); 

//		printf( "%f %f %f / %f %f %f / %f %f %f\n", wm0p.Pos[0], wm0p.Pos[1], wm0p.Pos[2], wm1p.Pos[0], wm1p.Pos[1], wm1p.Pos[2], phmd.Pos[0], phmd.Pos[1], phmd.Pos[2] );

		tdPush();
		//tdScale( gSMatrix, .2, .2, .2 );		//Operates ON f

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
			tdTranslate( gSMatrix, 0.0, 0, 1 );
			tdPush();
			for( y = 0; y < 5; y++ )
			{
				tdTranslate( gSMatrix, 0.0, 1, 0 );
				tdPush();
				for( x = 0; x < 5; x++ )
				{
					tdTranslate( gSMatrix, 1, 0, 0 );
					//int rstart = ((tframes)*6)%36;
					tdPush();
					//float sm = sin(x*1.2+y*.3+z*.8+tframes*.05);
					//tdScale( gSMatrix, sm, sm, sm );
					//tdRotateEA( gSMatrix, tframes+z*10, tframes*3.+y*10, tframes*2+x*1 );
					tdScale( gSMatrix, .1, .1, .1 );
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
