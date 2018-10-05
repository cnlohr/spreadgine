//For keyboard input.
#ifdef RASPI_GPU
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#endif


void * HandleTTYInput( void * d )
{
	int f = open( "/dev/tty0", O_RDONLY | O_SYNC );
	struct termios to;
	cfmakeraw( &to );
	tcsetattr( f, 0, &to );

	printf( "Got hardware tty: %d\n", f );
	while(1)
	{
		char c;
		int r = read( f, &c, 1 );
		//printf( "%d %d\n", r,c );
		//if( tbfocus ) TextBoxHandleKeyAscii( tbfocus, c, bDown );
		if( c == 126 ) c = '|';
		if( c == 194 ) c = '~';
		if( c == 172 ) continue;

		HandleKeypress( c );
		if( r < 0 ) return 0;
	}
}


