#include <spread_vr.h>
#include <string.h>


float disappearing = 0.05;
float diopter = 0.032;
float fovie = 73;
float eyez = -.08;

og_mutex_t poll_mutex;


int gargc;
char ** gargv;
SurvivePose phmdC;
SurvivePose phmd;
SurvivePose phmdlast; //Actually raw pose.
SurvivePose wmpC[2];
SurvivePose wmp[2];
SurvivePose wmplast[2];


Spreadgine * gspe;
SurviveObject * HMD;
SurviveObject * WM[2];
SurvivePose shift_gun, shift_hmd;
struct SurviveContext *survivectx;

void my_raw_pose_process(SurviveObject *so, uint32_t timecode, SurvivePose *pose)
{
        survive_default_raw_pose_process(so, timecode, pose);
	OGLockMutex(poll_mutex);

	if( strcmp( so->codename, "HMD" ) == 0 )
	  {
		memcpy( &phmdC, pose, sizeof( SurvivePose ) ); 
	    HMD = so;
	  }
	else if( strcmp( so->codename, "WM0" ) == 0 || strcmp( so->codename, "WM1" ) == 0 )
	  {
		int id = so->codename[2]-'0';
		memcpy( &wmpC[id], pose, sizeof( SurvivePose ) ); 
	    WM[id] = so;
	  }



	OGUnlockMutex(poll_mutex);
}


void * LibSurviveThread()
{       
        survivectx = survive_init(gargc,gargv);

        if (survivectx == 0) // implies -help or similiar
                return 0;

        survive_install_pose_fn(survivectx, my_raw_pose_process);

        survive_startup(survivectx);

        while (survive_poll(survivectx) == 0) {
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

	memcpy( &shift_gun, &LinmathPose_Identity, sizeof(LinmathPose_Identity) ); 
	LinmathEulerAngle euler = { -.15, 0, 0 };
	quatfromeuler( shift_gun.Rot, euler );

	memcpy( &shift_hmd, &LinmathPose_Identity, sizeof(LinmathPose_Identity) ); 
	LinmathEulerAngle euler2 = { -.00, 0, 0 };
	quatfromeuler( shift_hmd.Rot, euler2 );
}

void UpdateRots( SurvivePose * out, SurvivePose * last, SurvivePose * raw, SurvivePose * shift )
{
	LinmathQuat TempRot;

	//Find differential from last frame to this one.
	LinmathQuat invertedlast, differential_rotation;
	quatgetreciprocal( invertedlast, last->Rot );
	quatrotateabout( differential_rotation, invertedlast, raw->Rot );

	quatslerp( differential_rotation, LinmathQuat_Identity, differential_rotation, 2 ); //Account for latency and advance motion feed forward. 

	memcpy( out, raw, sizeof(SurvivePose) );
	quatrotateabout( out->Rot, out->Rot, differential_rotation );

	quatrotateabout( out->Rot, out->Rot, shift->Rot );
	//ApplyPoseToPose( out, shift, out );

	//Keep current value.
	memcpy( last, raw, sizeof( SurvivePose ) );
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

	/* Process positional updates */
	UpdateRots( &phmd, &phmdlast, &phmdC, &shift_hmd );
	UpdateRots( &wmp[0], &wmplast[0], &wmpC[0], &shift_gun );
	UpdateRots( &wmp[1], &wmplast[1], &wmpC[1], &shift_gun );

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






