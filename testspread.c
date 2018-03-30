#include <spreadgine.h>

int main()
{
	Spredgine * e = SpreadInit( 960, 640, "Spread Test", 8888, 2, stderr );

	while(1)
	{
		spglClearColor( e, 1., 0., 1., 1. );
		spglClear( e, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		spglSwap( e );

	}
}
