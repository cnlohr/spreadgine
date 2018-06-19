#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <math.h>
#include <spread_vr.h>

#define MAX_BOOLETS 1024
#define MAX_BLOCKS 64

float boolet_pos[MAX_BOOLETS*3];
float boolet_vec[MAX_BOOLETS*3];
float boolet_age[MAX_BOOLETS];

float blocksplode[MAX_BLOCKS];
float blockpos[MAX_BLOCKS*3];
float blockco[MAX_BLOCKS*3];


float boolets_arrayP[MAX_BOOLETS*6*2];
float boolets_arrayC[MAX_BOOLETS*8*2];
uint16_t boolets_ibo[MAX_BOOLETS*2*2];
int numboolets;
SpreadGeometry * boolets;

void HandleKey( int keycode, int bDown )
{
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

void HandleControllerInput()
{
	int id = 0;

	for( id = 0; id < 2; id++ )
	{
		struct SurviveObject * w = WM[id];
		if( !w ) continue;
		SurvivePose fvv = wmp[id];
		static int wasdown[2];
		int down = 0;
		if( w->axis1 > 30000 )
			down = 1;

		if( wasdown[id] == 0 && down )
		{
			FLT forward[3];
			FLT forwardin[3] = { 0, 0, 1 };
			quatrotatevector(forward, fvv.Rot, forwardin);
			FLT up[3];
			FLT upin[3] = { 0, 1, 0 };
			quatrotatevector(up, fvv.Rot, upin);

			boolet_pos[numboolets*3+0] = fvv.Pos[0]+up[0]*.05;
			boolet_pos[numboolets*3+1] = fvv.Pos[1]+up[1]*.05;
			boolet_pos[numboolets*3+2] = fvv.Pos[2]+up[2]*.05;

			boolet_vec[numboolets*3+0] = forward[0];
			boolet_vec[numboolets*3+1] = forward[1];
			boolet_vec[numboolets*3+2] = forward[2];
			boolet_age[numboolets] = 0.1;
			numboolets++;
		}
		wasdown[id] = down;

	}
}

void UpdateBoolets( float dtime )
{
	int i;
	for( i = 0; i < numboolets;i++ )
	{
			if( boolet_age[i] == 0 || boolet_age[i] > 20 ) {
				boolets_arrayC[i*8+0] = 0; 
				boolets_arrayC[i*8+1] = 0; 
				boolets_arrayC[i*8+2] = 0; 
				boolets_arrayC[i*8+3] = 0; 
				boolets_arrayC[i*8+4] = 0; 
				boolets_arrayC[i*8+5] = 0; 
				boolets_arrayC[i*8+6] = 0; 
				boolets_arrayC[i*8+7] = 0; 
			}
			boolets_arrayP[i*6+0] = boolet_pos[i*3+0]; 
			boolets_arrayP[i*6+1] = boolet_pos[i*3+1]; 
			boolets_arrayP[i*6+2] = boolet_pos[i*3+2]; 
			boolets_arrayP[i*6+3] = boolet_pos[i*3+0] + boolet_vec[i*3+0]*.2; 
			boolets_arrayP[i*6+4] = boolet_pos[i*3+1] + boolet_vec[i*3+1]*.2; 
			boolets_arrayP[i*6+5] = boolet_pos[i*3+2] + boolet_vec[i*3+2]*.2; 

			boolet_pos[i*3+0] += boolet_vec[i*3+0]*dtime*10;
			boolet_pos[i*3+1] += boolet_vec[i*3+1]*dtime*10;
			boolet_pos[i*3+2] += boolet_vec[i*3+2]*dtime*10;

			int j;
			for( j = 0; j < MAX_BLOCKS; j++ )
			{
				if( blocksplode[j] > 0.11 ) continue;

				float * bp = &boolet_pos[i*3];
				float * kp = &blockpos[j*3];
				float dist = ((bp[0]-kp[0])*(bp[0]-kp[0])) + ((bp[1]-kp[1])*(bp[1]-kp[1])) + ((bp[2]-kp[2])*(bp[2]-kp[2]));

				if( dist < 0.2*0.2 )
				{
					boolet_age[i] = 0;
					blocksplode[j] = 0.12;
				}
			}
			boolets_arrayC[i*8+0] = 1; 
			boolets_arrayC[i*8+1] = 1; 
			boolets_arrayC[i*8+2] = 1; 
			boolets_arrayC[i*8+3] = 1; 
			boolets_arrayC[i*8+4] = 1; 
			boolets_arrayC[i*8+5] = 0; 
			boolets_arrayC[i*8+6] = 0; 
			boolets_arrayC[i*8+7] = 1; 

	}
	UpdateSpreadGeometry( boolets, 0, boolets_arrayP );
	UpdateSpreadGeometry( boolets, 1, boolets_arrayC );

	int j;
	static double tt = 0;
	tt+=dtime;
	for( j = 0; j < MAX_BLOCKS; j++ )
	{
		if( blocksplode[j] > 0.11 ) continue;
		float * kp = &blockpos[j*3];
		float * vv = &blockco[j*3];

		kp[2] = vv[2];
		kp[0] = sin( vv[0] + tt*.3 )*vv[1]; 
		kp[1] = cos( vv[0] + tt*.3 )*vv[2]; 
	}
}

int main( int argc, char ** argv )
{
	int i;
	gspe = SpreadInit( 2160, 1200, "Spread Game Survive Test", 8888, 2, stderr );

	gargc = argc;
	gargv = argv;

	tdMode( tdMODELVIEW );

	SpreadGeometry * gun = LoadOBJ( gspe, "assets/simple_gun.obj", 1, 1 );

	{
		int strides[] = { 3, 4 };
		int types[] = {GL_FLOAT,GL_FLOAT};
		float * arrays[] = { 
			boolets_arrayP,
			boolets_arrayC};

		printf( "Making boolets\n" );
		int i;
		for( i = 0; i < MAX_BOOLETS*2; i++ )
		{
			boolets_ibo[i] = i;
		}
		boolets = SpreadCreateGeometry( gspe, "boolets", GL_LINES, MAX_BOOLETS*2, boolets_ibo, MAX_BOOLETS*2, 2, arrays, strides, types);
		printf( "Made boolets\n" );
		numboolets = 0;
	}

	for( i = 0; i < MAX_BLOCKS; i++ )
	{
		blockco[i*3+0] = blockpos[i*3+0] = rand()%10;
		blockco[i*3+1] = blockpos[i*3+1] = rand()%10;
		blockco[i*3+2] = blockpos[i*3+2] = rand()%10;
		blocksplode[i] = 0.1;
	}

//exit(-1);
	SpreadGeometry * platform = LoadOBJ( gspe, "assets/platform.obj", 1, 1 );
	//e->geos[0]->render_type = GL_LINES;
	UpdateSpreadGeometry( gspe->geos[0], -1, 0 );

	SpreadSetupVR();

	int x, y, z;

	int frames = 0, tframes = 0;
	double lastframetime = OGGetAbsoluteTime();
	double TimeSinceStart = 0;
	double Last = OGGetAbsoluteTime();
	double TimeOfLastSwap = OGGetAbsoluteTime();
	while(1)
	{
		double Now = OGGetAbsoluteTime();
		double Delta = Now - Last;
		spglClearColor( gspe, .1, 0.1, 0.1, 1.0 );

		HandleControllerInput();


		spglClear( gspe, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( gspe, GL_DEPTH_TEST );

		spglEnable( gspe, GL_CULL_FACE );

		SpreadSetupEyes();

		int x, y;

        tdMode( tdMODELVIEW );
        tdIdentity( gSMatrix );

		spglLineWidth( gspe, 4 );

		SpreadApplyShader( gspe->shaders[0] );


		tdPush();

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

		SpreadRenderGeometry( platform, gSMatrix, 0, -1 ); 

//		printf( "%f %f %f / %f %f %f / %f %f %f\n", wmp[0].Pos[0], wmp[0].Pos[1], wmp[0].Pos[2], wmp[1].Pos[0], wmp[1].Pos[1], wmp[1].Pos[2], phmd.Pos[0], phmd.Pos[1], phmd.Pos[2] );

		tdPush();
		//tdScale( gSMatrix, .2, .2, .2 );		//Operates ON f

		float ssf[4] = { TimeSinceStart, 0, 0, 0 };
		int slot = SpreadGetUniformSlot( gspe->shaders[0], "timevec");
		if( slot >= 0 )
		{
			//printf( "%f\n", ssf[0] );
			SpreadUniform4f( gspe->shaders[0], slot, ssf );
		}



		for( i = 0 ; i < MAX_BLOCKS; i++ )
		{
			if( blocksplode[i] > 1 ) continue;
			tdPush();
			tdTranslate( gSMatrix, blockpos[i*3+0], blockpos[i*3+1], blockpos[i*3+2] );
			//int rstart = ((tframes)*6)%36;
			tdPush();
			//float sm = sin(x*1.2+y*.3+z*.8+tframes*.05);
			//tdScale( gSMatrix, sm, sm, sm );
			tdScale( gSMatrix, blocksplode[i], blocksplode[i], blocksplode[i] );

			if( blocksplode[i] > 0.11 )
			{
				tdRotateEA( gSMatrix, blocksplode[i]*10000, blocksplode[i]*1000, 0);
				blocksplode[i] *= 1.01;
			}

			SpreadRenderGeometry( gspe->geos[0], gSMatrix, 0, -1 ); 

			tdPop();
		}

//		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		UpdateBoolets( Delta );
		SpreadRenderGeometry( boolets, gSMatrix, 0, numboolets*2 );
		tdPop();

#ifndef RASPI_GPU
		glFlush();
		double TWS = OGGetAbsoluteTime();
		TWS = TimeOfLastSwap-TWS + 1.f/90.f - .0001;
		if( TWS > 0 )	usleep(TWS*1000000);
		TimeOfLastSwap = OGGetAbsoluteTime();
#endif
		spglSwap( gspe );

		SpreadCheckShaders( gspe );
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
