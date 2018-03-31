#include <arpa/inet.h> //For htonl
#include <cnhttp.h>
#include <http_bsd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <os_generic.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>


void CloseEvent()
{
}

void * SpreadHTTPThread( void * spread )
{
	Spreadgine * spr;
	while( 1 )
	{
		//DO WORK HERE
		//Also figure out a graceful way of quitting when spreadgine wants to shut down.
		//...

	}
}

void HTTPCustomStart( )
{
}

void NewWebSocket()
{
}

void WebSocketData( int len )
{
}

void WebSocketTick()
{
}

void HTTPCustomCallback( )
{
}


void SpreadPushMessage( Spreadgine * e, uint8_t messageid, int payloadsize, void * payload )
{
	if( payloadsize > SPREADGINE_CIRCBUF/2 )
	{
		fprintf( e->fReport, "Error pushing message %d.  Size: %d\n", messageid, payloadsize );
		return;
	}
	int modhead = e->cbhead % SPREADGINE_CIRCBUF;
	int sent = 0;

	e->cbbuff[modhead] = payloadsize>>24; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>16; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>8; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>0; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;

	e->cbbuff[modhead] = messageid;
	modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;

	int endmod = modhead + payloadsize;
	if( endmod > SPREADGINE_CIRCBUF )
	{
		int remain = SPREADGINE_CIRCBUF - modhead;
		memcpy( e->cbbuff + modhead, payload, remain );
		memcpy( e->cbbuff, payload + remain, payloadsize - remain );
	}
	else
	{
		memcpy( e->cbbuff + modhead, payload, payloadsize );
	}
	sent += payloadsize;

	e->cbhead += sent;

}



//"entry" is in printf mode... "format" is in 'b' for byte, 'i' for integer, 'f' for float, 's' for string, 'v' takes two parameters, a # of bytes and a pointer to the payload.
void SpreadMessage( Spreadgine * e, const char * entry, const char * format, ... )
{
	//XXX TODO:  This does an allocation every time it sends a message. Consider changing behavior so that doesn't happen.

    va_list ap;
    va_start(ap, format);
	struct SpreadHashEntry * he;
	static struct SpreadHashEntry heheap;	//Copy of an entry on the heap we can use if 'entry' is null. (WARNING: NOT THREADSAFE)

	if( entry )
	{
		char namebuffer[128];
		int vsnpf = vsnprintf( namebuffer, sizeof(namebuffer), entry, ap );
		if( vsnpf <= 0 )
		{
			fprintf( e->fReport, "Error: SpreadMessage called with invalid entry formatting.\n" );
			return;
		}
		OGLockMutex( e->KEEPmutex );
		he = SpreadHashEntryGetOrInsert( e, namebuffer  );
	}
	else
	{
		he = &heheap;
		memset( he, 0, sizeof(heheap) );
	}

	int outplace = 4;	//Reserve first 4 bytes for packet length.
	char c, d;
	int i, j;
	int v;
	void * vp;
	float f;
	const char * s;
	const char ** sv;

	if( outplace > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 

	for( i = 0; c = format[i++]; )
	{
		switch( c )
		{
		case 'b':
			if( outplace + 1 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
			he->payload[outplace++] = va_arg(ap, int);
			break;
		case 'i':
			if( outplace + 4 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
			v = va_arg(ap, int);
			he->payload[outplace++] = v>>24;
			he->payload[outplace++] = v>>16;
			he->payload[outplace++] = v>>8;
			he->payload[outplace++] = v>>0;
			break;
		case 'f':
			if( outplace + 4 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
			f = va_arg(ap, double );
			memcpy( &he->payload[outplace], &f, 4 );
			break;
		case 'S':
			v = va_arg(ap, int);
			sv = va_arg(ap, const char ** );
			if( outplace + 4 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
			he->payload[outplace++] = v>>24;
			he->payload[outplace++] = v>>16;
			he->payload[outplace++] = v>>8;
			he->payload[outplace++] = v>>0;
			for( j = 0; j < v; j++ )
			{
				s = sv[j];
				while( d = *(s++) )
				{
					if( outplace + 1 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
					he->payload[outplace++] = d;
				}
			}
			break;
		case 's':
			s = va_arg(ap, const char * );
			while( d = *(s++) )
			{
				if( outplace + 1 > he->payload_reserved ) he->payload = realloc( he->payload,he->payload_reserved+=64 ); 
				he->payload[outplace++] = d;
			}
			break;
		case 'v':
			v = va_arg(ap, int );
			vp = va_arg(ap, void * );
			if( outplace + v + 4 > he->payload_reserved ) he->payload = realloc( he->payload,he->payload_reserved+=64+v ); 
			he->payload[outplace++] = v>>24;
			he->payload[outplace++] = v>>16;
			he->payload[outplace++] = v>>8;
			he->payload[outplace++] = v>>0;
			memcpy( he->payload + outplace, vp, v );
			break;			
		}
	}

	he->payloadlen = outplace;
	uint32_t nbo = htonl( outplace );
	memcpy( he->payload, &nbo, 4 );

	printf ( "%d: ", he->payloadlen );
	for( i = 0 ; i < he->payloadlen; i++ )
	{
		printf( "%02x ", he->payload[i] );
	}
	printf( "\n" );

	int modhead = e->cbhead % SPREADGINE_CIRCBUF;
	int sent = 0;
	int endmod = modhead + he->payloadlen;

	if( endmod > SPREADGINE_CIRCBUF )
	{
		int remain = SPREADGINE_CIRCBUF - modhead;
		memcpy( e->cbbuff + modhead, he->payload, remain );
		memcpy( e->cbbuff, he->payload + remain, he->payloadlen - remain );
	}
	else
	{
		memcpy( e->cbbuff + modhead, he->payload, he->payloadlen );
	}
	sent += he->payloadlen;
	e->cbhead += sent;	//Advance pointer.

	if( he != &heheap )
	{
		OGUnlockMutex( e->KEEPmutex );
	}
}

void SpreadRemoteInit( Spreadgine * e )
{
	e->KEEPmutex = OGCreateMutex();
	e->spreadthread = OGCreateThread( SpreadHTTPThread, e );
}




static unsigned long case_matters_djb_hashl(const char *clave)
{
    unsigned long c,i,h;

    for(i=h=0;c=clave[i];i++)
    {
        h = ((h << 5) + h) ^ c;
    }
    return h;
}

struct SpreadHashEntry * SpreadHashEntryGetOrInsert( Spreadgine * e , const char * he )
{
	int hashval = case_matters_djb_hashl( he ) % SPREADGINE_CACHEMAP_SIZE;
	struct SpreadHashEntry ** hashbin = &e->KEEPhash[hashval];

	while( *hashbin )
	{
		if( strcmp( (*hashbin)->key, he ) == 0 )
			return *hashbin;
		hashbin = &((*hashbin)->next);
	}

	struct SpreadHashEntry * ne = (*hashbin) = malloc( sizeof( struct SpreadHashEntry ) );
	ne->key = strdup( he );
	ne->payloadlen = 0;
	ne->payload_reserved = 0;
	ne->payload = 0;
	ne->next = 0;
	return ne;
}

void SpreadHashRemove( Spreadgine * e, const char * he )
{
	int hashval = case_matters_djb_hashl( he ) % SPREADGINE_CACHEMAP_SIZE;
	struct SpreadHashEntry * hashbin = e->KEEPhash[hashval];
	while( hashbin )
	{
		if( strcmp( hashbin->key, he ) == 0 )
		{
			free( hashbin->key );
			if( hashbin->payload ) free( hashbin->payload );
			free( hashbin );
			return;
		}
		hashbin = hashbin->next;
	}
	return;
}

