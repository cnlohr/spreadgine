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

void HandleKey( int keycode, int bDown )
{
	if( tbfocus ) TextBoxHandleKeyX11( tbfocus, keycode, bDown );
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
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
/*
		//Example test code
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
*/
		}
		wasdown[id] = down;
	}
}


int main( int argc, char ** argv )
{
	int i;
	gspe = SpreadInit( 2160, 1200, "TCCEnvironment for Spreadgine", 8888, 2, stderr );

#ifdef RASPI_GPU
	OGCreateThread( HandleTTYInput, 0 );
#endif

	gargc = argc;
	gargv = argv;

	tdMode( tdMODELVIEW );

	SpreadGeometry * gun = LoadOBJ( gspe, "assets/simple_gun.obj", 1, 1 );

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

	SpreadGeometry * platform = LoadOBJ( gspe, "assets/platform.obj", 1, 1 );
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
