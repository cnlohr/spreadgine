#include <spread_vr.h>
#include <string.h>


float disappearing = 0.08;
float diopter = 0.032;
float fovie = 80;
float eyez = -.08;

og_mutex_t poll_mutex;


int gargc;
char ** gargv;
SurvivePose phmd;
SurvivePose wm0p;
SurvivePose wm1p;
Spreadgine * gspe;
SurviveObject * HMD;
SurviveObject * WM0;
SurviveObject * WM1;

void my_raw_pose_process(SurviveObject *so, uint32_t timecode, SurvivePose *pose)
{
        survive_default_raw_pose_process(so, timecode, pose);
	OGLockMutex(poll_mutex);

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
	OGUnlockMutex(poll_mutex);

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


void SpreadSetupVR()
{
	poll_mutex = OGCreateMutex();
	OGCreateThread( LibSurviveThread, gspe );
#ifdef RASPI_GPU
	int act_w = 1024;
#else
	int act_w = 1080;
#endif
	SpreadSetupCamera( gspe, 0, fovie, (float)act_w/1200, .01, 1000, "CAM0" );
	SpreadSetupCamera( gspe, 1, fovie, (float)act_w/1200, .01, 1000, "CAM1" );
}

//disappearing, diopter, fovie, eyez

void SpreadSetupEyes()
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

	if( HMD )
	{
		diopter = HMD->axis1 / 200000.0f;
		//printf( "%d %d %d\n", HMD->axis1, HMD->axis2, HMD->axis3 );
	}

	OGLockMutex(poll_mutex);
	ApplyPoseToPoint(p, &phmd, pin);
	ApplyPoseToPoint(eye1, &phmd, pineye1);
	ApplyPoseToPoint(eye2, &phmd, pineye2);

	ApplyPoseToPoint(at1, &phmd, pinat1);
	ApplyPoseToPoint(at2, &phmd, pinat2);

	ApplyPoseToPoint(up, &phmd, pinup);
	OGUnlockMutex(poll_mutex);	
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






