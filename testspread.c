#include <spreadgine.h>
#include <CNFG3D.h>
#include <unistd.h>

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
	Spreadgine * e = SpreadInit( 960, 640, "Spread Test", 8888, 2, stderr );

	tdTranslate( e->vpviews[0], -.5, 0, 0 );
	tdTranslate( e->vpviews[1], .5, 0, 0 );
	e->geos[0].render_type = GL_TRIANGLES;
	SpreadChangeCameaView(e, 0, e->vpviews[0] );
	SpreadChangeCameaView(e, 1, e->vpviews[1] );

	float modelmatrix[16];
	tdIdentity( modelmatrix );
	tdTranslate( modelmatrix, 0., 0., -5. );
	float f;
	while(1)
	{
		spglClearColor( e, .2, 0.2, 0.2, 1.0 );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglEnable( e, GL_DEPTH_TEST );
		SpreadApplyShader( &e->shaders[0] );
		SpreadRenderGeometry( &e->geos[0], 0, 36, modelmatrix ); 

		tdRotateEA( modelmatrix, 0,1,.2125 );		//Operates ON f
		//tdTranslate( modelmatrix, 0, 0, .1 );

		usleep(10000);
		spglSwap( e );
	}
}
