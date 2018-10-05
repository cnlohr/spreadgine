#include <CNFG3D.h>
#include <unistd.h>
#include <os_generic.h>
#include <string.h>
#include <math.h>
#include <spreadgine_vr.h>
#include <spreadgine_util.h>
#include <spreadgine_remote.h>
#include <stdarg.h>
#include "modules/textboxes.h"
#include "modules/ttyinput.h"

TextBox * tbfocus;

void HandleKeypress( int ch )
{
	printf( "%d\n", ch );
	if( tbfocus ) TextBoxHandleKeyAscii( tbfocus, ch, 1 );
}

#define MAX_LINES   2048
#define MAX_BOOLETS 1024
#define MAX_BLOCKS 20

float boolet_pos[MAX_BOOLETS*3];
float boolet_vec[MAX_BOOLETS*3];
float boolet_age[MAX_BOOLETS];

float blocksplode[MAX_BLOCKS];
float blockpos[MAX_BLOCKS*3];
float blockspeed[MAX_BLOCKS*3];
float blockco[MAX_BLOCKS*3];

int boolet_in_use[MAX_BOOLETS];
float boolet_speed[MAX_BOOLETS];
float boolets_arrayP[MAX_LINES*6*2];
float boolets_arrayC[MAX_LINES*8*2];
uint16_t boolets_ibo[MAX_LINES*2*2];
int freebid;
SpreadGeometry * boolets;




int PerLinePlace = 0;
int ShotsPerGun[2];
void ClearPerFrameLines()
{
	//memset( &boolets_arrayP[MAX_LINES*6], 0, sizeof(MAX_LINES*6)*4 ); 
	PerLinePlace = MAX_LINES;
}

void AddPerLinePlace( float * mat, float * pt1, float * pt2, float * c1, float * c2 )
{
	tdPTransform( pt1, mat, &boolets_arrayP[PerLinePlace*3+0] );
	tdPTransform( pt2, mat, &boolets_arrayP[PerLinePlace*3+3] );
	memcpy( &boolets_arrayC[PerLinePlace*4+0], c1, sizeof( float ) * 4 );
	memcpy( &boolets_arrayC[PerLinePlace*4+4], c2, sizeof( float ) * 4 );
	PerLinePlace +=2;
}

extern const unsigned short FontCharMap[256];
extern const unsigned char FontCharData[1902];

void RawDrawText( float scale, float * mat, float * color, const char * textf, ... )
{
	const unsigned char * lmap;
	float PenX = 0;
	float PenY = 0;
	char text[1024];


	va_list args;
	va_start (args, textf);
	vsprintf (text, textf, args);
	va_end (args);

	float iox = PenX;//(float)CNFGPenX;
	float ioy = PenY;//(float)CNFGPenY;

	int place = 0;
	unsigned short index;
	int bQuit = 0;
	while( text[place] )
	{
		unsigned char c = text[place];

		switch( c )
		{
		case 9:
			iox += 12 * scale;
			break;
		case 10:
			iox = PenX;
			ioy += 6 * scale;
			break;
		default:
			index = FontCharMap[c];
			if( index == 65535 )
			{
				iox += 3 * scale;
				break;
			}

			lmap = &FontCharData[index];
			do
			{
				float x1 = (float)((((*lmap) & 0x70)>>4)*scale + iox);
				float y1 = (float)(((*lmap) & 0x0f)*scale + ioy);
				float x2 = (float)((((*(lmap+1)) & 0x70)>>4)*scale + iox);
				float y2 = (float)(((*(lmap+1)) & 0x0f)*scale + ioy);
				lmap++;
				float pt1[3] = { x1, y1, 0 };
				float pt2[3] = { x2, y2, 0 };
				AddPerLinePlace( mat, pt1, pt2, color, color );
				//CNFGTackSegment( x1, y1, x2, y2 );
				bQuit = *lmap & 0x80;
				lmap++;
			} while( !bQuit );

			iox += 3 * scale;
		}
		place++;
	}
}



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
		if( down )
		{
			FLT forward[3];
			FLT forwardin[3] = { 0, 0, 1 };
			quatrotatevector(forward, fvv.Rot, forwardin);
			FLT up[3];
			FLT upin[3] = { 0, 1, 0 };
			quatrotatevector(up, fvv.Rot, upin);

			boolet_pos[freebid*3+0] = fvv.Pos[0]+up[0]*.05;
			boolet_pos[freebid*3+1] = fvv.Pos[1]+up[1]*.05;
			boolet_pos[freebid*3+2] = fvv.Pos[2]+up[2]*.05;

			boolet_vec[freebid*3+0] = forward[0];
			boolet_vec[freebid*3+1] = forward[1];
			boolet_vec[freebid*3+2] = forward[2];
			boolet_age[freebid] = 0.1;
			boolet_in_use[freebid] = 1;
			boolet_speed[freebid] = 10;
			freebid = (freebid+1)%MAX_BOOLETS;

			ShotsPerGun[id]++;
		}
		wasdown[id] = down;

	}
}

void UpdateBoolets( float dtime )
{
	int i;

	for( i = 0; i < MAX_BOOLETS;i++ )
	{

		if( !boolet_in_use[i] ) continue;
		if( boolet_age[i] == 0 || boolet_age[i] > 20 ) {
			boolets_arrayC[i*8+0] = 0; 
			boolets_arrayC[i*8+1] = 0; 
			boolets_arrayC[i*8+2] = 0; 
			boolets_arrayC[i*8+3] = 0; 
			boolets_arrayC[i*8+4] = 0; 
			boolets_arrayC[i*8+5] = 0; 
			boolets_arrayC[i*8+6] = 0; 
			boolets_arrayC[i*8+7] = 0; 
			
			boolet_in_use[i] = 0;
			continue;
		}
		boolet_age[i] += dtime;


		boolets_arrayP[i*6+0] = boolet_pos[i*3+0]; 
		boolets_arrayP[i*6+1] = boolet_pos[i*3+1]; 
		boolets_arrayP[i*6+2] = boolet_pos[i*3+2]; 
		boolets_arrayP[i*6+3] = boolet_pos[i*3+0] + boolet_vec[i*3+0]*.2; 
		boolets_arrayP[i*6+4] = boolet_pos[i*3+1] + boolet_vec[i*3+1]*.2; 
		boolets_arrayP[i*6+5] = boolet_pos[i*3+2] + boolet_vec[i*3+2]*.2;

		if( boolet_pos[i*3+2] < 0 ) boolet_speed[i] = 0;

		boolet_pos[i*3+0] += boolet_vec[i*3+0]*dtime*boolet_speed[i];
		boolet_pos[i*3+1] += boolet_vec[i*3+1]*dtime*boolet_speed[i];
		boolet_pos[i*3+2] += boolet_vec[i*3+2]*dtime*boolet_speed[i];
		boolet_vec[i*3+2] -= dtime*.02*boolet_speed[i];

		if( boolet_pos[i*3+2] <= 0.01 ) continue;

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
				boolet_in_use[i] = 0;
			}
		}

	}
	UpdateSpreadGeometry( boolets, 0, boolets_arrayP );

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

void ResetBlock( int i )
{
	blockco[i*3+0] = blockpos[i*3+0] = rand()%10;
	blockco[i*3+1] = blockpos[i*3+1] = rand()%10;
	blockco[i*3+2] = blockpos[i*3+2] = rand()%10;
	blocksplode[i] = 0.1;
}

int main( int argc, char ** argv )
{
	int i;
	gspe = SpreadInit( 2160, 1200, "Spread Game Survive Test", 8888, 2, stderr );

#ifdef RASPI_GPU
	OGCreateThread( HandleTTYInput, 0 );
#endif

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
		for( i = 0; i < MAX_LINES*2; i++ )
		{
			boolets_ibo[i] = i;
		}

		boolets = SpreadCreateGeometry( gspe, "boolets", GL_LINES, MAX_LINES*2, boolets_ibo, MAX_LINES*2, 2, (const void**)arrays, strides, types);


		for( i = 0; i < MAX_LINES/2; i++ )
		{
			boolets_arrayC[i*8+0] = 1; 
			boolets_arrayC[i*8+1] = 1; 
			boolets_arrayC[i*8+2] = 1; 
			boolets_arrayC[i*8+3] = 1; 
			boolets_arrayC[i*8+4] = 1; 
			boolets_arrayC[i*8+5] = 0; 
			boolets_arrayC[i*8+6] = 0; 
			boolets_arrayC[i*8+7] = 1; 
		}
		for( ; i < MAX_LINES; i++ )
		{
			boolets_arrayC[i*8+0] = 1; 
			boolets_arrayC[i*8+1] = 1; 
			boolets_arrayC[i*8+2] = 1; 
			boolets_arrayC[i*8+3] = 1; 
			boolets_arrayC[i*8+4] = 1; 
			boolets_arrayC[i*8+5] = 1; 
			boolets_arrayC[i*8+6] = 1; 
			boolets_arrayC[i*8+7] = 1; 
		}

		UpdateSpreadGeometry( boolets, 1, boolets_arrayC );
		printf( "Made boolets\n" );
	}

	#define NUM_TEXTBOXES 1
	TextBoxSet * textboxes =  CreateTextBoxSet( gspe, "cntools/vlinterm/ibm437.pgm", 25, 1024, 1024 );
	TextBox * textboxset[NUM_TEXTBOXES];
	for( i = 0; i < NUM_TEXTBOXES; i++ )
	{
		textboxset[i] = CreateTextBox( textboxes, "first", 80, 25 );
		char * localargv[] = { "/bin/bash", 0 };
		TextboxAttachTerminal( textboxset[i], localargv );
		tbfocus = textboxset[i];
	}


	for( i = 0; i < MAX_BLOCKS; i++ )
	{
		ResetBlock( i );
	}

//exit(-1);
	SpreadGeometry * platform = LoadOBJ( gspe, "assets/platform.obj", 1, 1 );
	//e->geos[0]->render_type = GL_LINES;
	UpdateSpreadGeometry( gspe->geos[0], -1, 0 );

	SpreadSetupVR();

	float Color[4] = { 1, 1, 1, 1 };
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
		spglClearColor( gspe, .01, 0.01, 0.01, 1.0 );

		HandleControllerInput();


		spglClear( gspe, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( gspe, GL_DEPTH_TEST );

		spglEnable( gspe, GL_CULL_FACE );

		SpreadSetupEyes();

		int x, y;

		ClearPerFrameLines();

        tdMode( tdMODELVIEW );
        tdIdentity( gSMatrix );

		spglLineWidth( gspe, 4 );

		SpreadApplyShader( gspe->shaders[0] );

		float ssf[4] = { TimeSinceStart, 0, 0, 0 };
		int slot = SpreadGetUniformSlot( gspe->shaders[0], "timevec");
		if( slot >= 0 )
		{
			//printf( "%f\n", ssf[0] );
			SpreadUniform4f( gspe->shaders[0], slot, ssf );
		}

		tdPush();

		int i;
		//Draw watchmen & lighthouses
		for( i = 0; i < 2; i++ )
		{
			tdPush();
			tdTranslate( gSMatrix, wmp[i].Pos[0], wmp[i].Pos[1], wmp[i].Pos[2] );
			tdRotateQuat( gSMatrix, wmp[i].Rot[0], wmp[i].Rot[1], wmp[i].Rot[2], wmp[i].Rot[3] );
			SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 

			RawDrawText( -.01, gSMatrix, Color, "GUN %d\nSHOT %d", i, ShotsPerGun[i] );

			tdPop();


			if( survivectx )
			{
				SurvivePose * ps = &survivectx->bsd[i].Pose;

				tdPush();
				tdTranslate( gSMatrix, ps->Pos[0], ps->Pos[1], ps->Pos[2] );
				tdRotateQuat( gSMatrix, ps->Rot[0], ps->Rot[1], ps->Rot[2], ps->Rot[3] );
				tdScale( gSMatrix, 2, 2, -2 );
				SpreadRenderGeometry( gun, gSMatrix, 0, -1 );
 
				RawDrawText( -.02, gSMatrix, Color, "LH %d", i );

				tdPop();
			}
		}

//		SpreadRenderGeometry( gun, gSMatrix, 0, -1 ); 
		UpdateBoolets( Delta );


//void RawDrawText( const char * text, int scale, float * mat )
		SpreadRenderGeometry( boolets, gSMatrix, 0, MAX_LINES*2);



		for( i = 0 ; i < MAX_BLOCKS; i++ )
		{
			if( blocksplode[i] > 1 )
			{
				ResetBlock( i );
			}
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


		{
			for( i = 0; i < NUM_TEXTBOXES; i++ )
			{
				double euler[3] = {-1.57, 0, 0 };
				LinmathQuat q;
				quatfromeuler( q, euler );
	
				float quat[4] = { q[0], q[1], q[2], q[3] }; 
				UpdateBatchedObjectTransformData( textboxset[i]->obj, 
					FQuad( 0, 1, i+1, .02 ),
					quat, 
					0 );
			}
			spglDisable( gspe, GL_CULL_FACE );
			RenderTextBoxSet( textboxes, gSMatrix);
			spglEnable( gspe, GL_CULL_FACE );
		}


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
		//enable_spread_remote = !enable_spread_remote;
	}
}
