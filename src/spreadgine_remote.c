#include <arpa/inet.h> //For htonl
#include <cnhttp.h>
#include <http_bsd.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <os_generic.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>

static Spreadgine * SpreadForHTTP;
int enable_spread_remote = 1;
void HTTPClose();

struct ClientStruct
{
	uint8_t * KEEPbufferout;
	uint32_t  KEEPplace;
	uint32_t  KEEPlength;


	uint32_t CircPtr;
};

static void huge()
{
	uint8_t i = 0;

	DataStartPacket();
	do
	{
		PushByte( 0 );
		PushByte( i );
	} while( ++i ); //Tricky:  this will roll-over to 0, and thus only execute 256 times.

	EndTCPWrite( curhttp->socket );
}


void HTTPCustomStart( )
{
	if( strncmp( (const char*)curhttp->pathbuffer, "/d/huge", 7 ) == 0 )
	{
		curhttp->rcb = (void(*)())&huge;
		curhttp->bytesleft = 0xffffffff;
	}
	else
	{
		curhttp->rcb = 0;
		curhttp->bytesleft = 0;
	}
	curhttp->isfirst = 1;
	HTTPHandleInternalCallback();

}

void CloseEvent()
{
	//No close event (no one cares when a client is done)
	if( curhttp->is_dynamic )
	{
		if( curhttp->data.userptr.v ) free( curhttp->data.userptr.v );
		curhttp->data.userptr.v = 0;
	}
}


static void WSStreamData(  int len )
{
	char cbo[len];
	int i;
	for( i = 0; i < len; i++ )
	{
		cbo[i] = WSPOPMASK();
	}
}


static void WSStreamOut( )
{
	int i;
	struct ClientStruct * cs = curhttp->data.userptr.v;
	if( !cs ) return;

	//Keep flowing the KEEP buffer out, until we're done with it
	if( cs->KEEPbufferout )
	{
		int rtor = cs->KEEPlength - cs->KEEPplace;
		int tosend = (rtor>1024)?1024:rtor;
		WebSocketSend( cs->KEEPbufferout + cs->KEEPplace, tosend );
		cs->KEEPplace += tosend;
		if(cs->KEEPplace ==  cs->KEEPlength )
		{
			free( cs->KEEPbufferout );
			cs->KEEPbufferout = 0;
		}	
	}
	else
	{
		//Then switch over to the circular buffer and pick up where we started sending the keep buffer.
		uint32_t tos = SpreadForHTTP->cbhead - cs->CircPtr;
		if( tos > SPREADGINE_CIRCBUF )
		{
			fprintf( SpreadForHTTP->fReport, "Error: Client would underflow circ buffer [%d %d %d]\n", tos, SpreadForHTTP->cbhead, cs->CircPtr );
			HTTPClose();
			return;
		}
		uint32_t tplace = SpreadForHTTP->cbhead % SPREADGINE_CIRCBUF;
		uint32_t splace = cs->CircPtr % SPREADGINE_CIRCBUF;
		uint32_t tosend = 0;

		//Would break boundary of end of circular buffer.  Don't do it.
		if( tplace < splace )
		{
			tosend = SPREADGINE_CIRCBUF-splace;
		}
		else
		{
			tosend = tplace-splace;
		}
		if( tosend > tos ) tosend = tos;
		if( tosend > 1024 ) tosend = 1024;
		//printf( "TOS %d/%d/%d [%d/%d] %d %d\n", tplace, splace, tosend, SpreadForHTTP->cbhead, cs->CircPtr, SPREADGINE_CIRCBUF, tos );
		if( tosend )
		{
			//printf( "%d [@%d]: ", tosend, splace ); for( i = 0; i < tosend; i++ ) printf( "%02x ", SpreadForHTTP->cbbuff[i+ splace] );
			WebSocketSend( SpreadForHTTP->cbbuff + splace, tosend );
			cs->CircPtr += tosend;
		}
	}
}


void NewWebSocket()
{
	if( strcmp( (const char*)curhttp->pathbuffer, "/d/ws/streamdata" ) == 0 )
	{
		curhttp->rcb = (void*)&WSStreamOut;
		curhttp->rcbDat = (void*)&WSStreamData;

		struct ClientStruct * c = curhttp->data.userptr.v = malloc( sizeof( struct ClientStruct ) );
		c->CircPtr = SpreadForHTTP->cbhead;
		c->KEEPplace = 0;
		c->KEEPlength = SpreadCreateDump( SpreadForHTTP, (uint8_t**)&c->KEEPbufferout );
	}
	else
	{
		curhttp->is404 = 1;
	}

}

void WebSocketData( int len )
{
	if( curhttp->rcbDat )
	{
		((void(*)( int ))curhttp->rcbDat)(  len ); 
	}
}

void WebSocketTick()
{
	if( curhttp->rcb )
	{
		((void(*)())curhttp->rcb)();
	}
}




void * SpreadHTTPThread( void * s )
{
	Spreadgine * spread = (Spreadgine*)s;
	while( !spread->doexit  )
	{
		TickHTTP();
		usleep( 3000 );
		//DO WORK HERE
		//Also figure out a graceful way of quitting when spreadgine wants to shut down.
		//...
	}
}


void HTTPCustomCallback( )
{
	if( curhttp->rcb )
		((void(*)())curhttp->rcb)();
	else
		curhttp->isdone = 1;
}


void SpreadPushMessage( Spreadgine * e, uint8_t messageid, int payloadsize, void * payload )
{
	if( !enable_spread_remote ) return;
	if( payloadsize > SPREADGINE_CIRCBUF/2 )
	{
		fprintf( e->fReport, "Error pushing message %d.  Size: %d\n", messageid, payloadsize );
		return;
	}
	uint32_t modhead = e->cbhead % SPREADGINE_CIRCBUF;
	int sent = 0;

	payloadsize++;//For mesageid.
	e->cbbuff[modhead] = payloadsize>>24; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>16; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>8; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	e->cbbuff[modhead] = payloadsize>>0; modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;
	payloadsize--;

	e->cbbuff[modhead] = messageid;
	modhead = (modhead+1)%SPREADGINE_CIRCBUF; sent++;

	int endmod = modhead + payloadsize;
	if( endmod > SPREADGINE_CIRCBUF )
	{
		int remain = SPREADGINE_CIRCBUF - modhead;
		memcpy( e->cbbuff + modhead, payload, remain );
		memcpy( e->cbbuff, payload + remain, payloadsize - remain );
	}
	else if(payloadsize)
	{
		memcpy( e->cbbuff + modhead, payload, payloadsize );
	}
	sent += payloadsize;

	e->cbhead += sent;

}



//"entry" is in printf mode... "format" is in 'b' for byte, 'i' for integer, 'f' for float, 's' for string, 'v' takes two parameters, a # of bytes and a pointer to the payload.
void SpreadMessage( Spreadgine * e, const char * entry, const char * format, ... )
{
	if( !enable_spread_remote ) return;
	//XXX TODO:  This does an allocation every time it sends a message. Consider changing behavior so that doesn't happen.

    va_list ap;
    va_start(ap, format);
	struct SpreadHashEntry * he;
	static struct SpreadHashEntry heheap;	//Copy of an entry on the heap we can use if 'entry' is null. (WARNING: NOT THREADSAFE)

	if( entry )
	{
		char namebuffer[128];
		int i, j = 0;
		char c;
		for( i = 0; (c = entry[i]) && j < sizeof(namebuffer)-12; i++ )
		{
			//We process the entry, and replace all #'s with things off the varargs stack.
			if( c == '#' )
			{
				j+=sprintf( namebuffer+j, "%d", va_arg(ap, int) );
			}
			else
			{
				namebuffer[j++] = c;
			}
		}
		namebuffer[j] = 0;
		//int vsnpf = vsnprintf( namebuffer, sizeof(namebuffer), entry, ap );
		//if( vsnpf <= 0 )
		//{
		//	fprintf( e->fReport, "Error: SpreadMessage called with invalid entry formatting.\n" );
		//	return;
		//}
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
			outplace += 4;
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
				do
				{
					d = *(s++);
					if( outplace + 1 > he->payload_reserved ) he->payload = realloc( he->payload, he->payload_reserved+=64 ); 
					he->payload[outplace++] = d;
				}
				while( d );
			}
			break;
		case 's':
			s = va_arg(ap, const char * );
			do
			{
				d = *(s++);
				if( outplace + 1 > he->payload_reserved ) he->payload = realloc( he->payload,he->payload_reserved+=64 ); 
				he->payload[outplace++] = d;
			} while( d );
			break;
		case 'X':	//Dump a specific amount of data into the output, raw.
			v = va_arg(ap, int );
			vp = va_arg(ap, void * );
			if( outplace + v> he->payload_reserved ) he->payload = realloc( he->payload,he->payload_reserved+=64+v ); 
			memcpy( he->payload + outplace, vp, v );
			outplace += v;
			break;
		case 'v':	//dump a specific amount of data into the output and include size.
			v = va_arg(ap, int );
			vp = va_arg(ap, void * );
			if( outplace + v + 4 > he->payload_reserved ) he->payload = realloc( he->payload,he->payload_reserved+=64+v ); 
			he->payload[outplace++] = v>>24;
			he->payload[outplace++] = v>>16;
			he->payload[outplace++] = v>>8;
			he->payload[outplace++] = v>>0;
			memcpy( he->payload + outplace, vp, v );
			outplace += v;
			break;			
		}
	}

	he->payloadlen = outplace;
	uint32_t nbo = htonl( he->payloadlen-4 );
	memcpy( he->payload, &nbo, 4 );
#if 0
	printf ( "%d: ", he->payloadlen );
	for( i = 0 ; i < he->payloadlen; i++ )
	{
		printf( "%02x[%c] ", he->payload[i], he->payload[i] );
	}
	printf( "\n" );
#endif
	uint32_t modhead = e->cbhead % SPREADGINE_CIRCBUF;
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

void SpreadRemoteInit( Spreadgine * e, int port )
{
	e->KEEPmutex = OGCreateMutex();
	RunHTTP( 8888 );
	e->spreadthread = OGCreateThread( SpreadHTTPThread, e );
	SpreadForHTTP = e;
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

int HashIDFromName( const char * he )
{
	return (he[0] == '-')?0:((he[0]=='+')?2:1);
}

struct SpreadHashEntry * SpreadHashEntryGetOrInsert( Spreadgine * e , const char * he )
{
	int hashval = case_matters_djb_hashl( he ) % SPREADGINE_CACHEMAP_SIZE;
	int hashid = HashIDFromName( he );
	struct SpreadHashEntry ** hashbin = &e->KEEPhash[hashid][hashval];

	while( *hashbin )
	{
		if( strcmp( (*hashbin)->key, he ) == 0 )
			return *hashbin;
		hashbin = &((*hashbin)->next);
	}

	if( e->KEEPlistnum[hashid] > SPREADGINE_MAXKEEP )
	{
		fprintf( e->fReport, "Error: Too many KEEP elements attached.\n" );
		return 0;
	}
	struct SpreadHashEntry * ne = (*hashbin) = malloc( sizeof( struct SpreadHashEntry ) );
	ne->key = strdup( he );
	ne->payloadlen = 0;
	ne->payload_reserved = 0;
	ne->payload = 0;
	ne->next = 0;
	int kel = e->KEEPlistnum[hashid]++;
	ne->entry_in_KEEPlist = kel;
	e->KEEPlist[hashid][kel] = ne;

	return ne;
}

void SpreadHashRemove( Spreadgine * e, const char * he, ... )
{
	
    va_list ap;
    va_start(ap, he);

	char hebuff[1024];
	int r = vsnprintf( hebuff, sizeof( hebuff ), he, ap );
	if( r <= 0 )
	{
		fprintf( e->fReport, "Error: SpreadHashRemove called with bad data\n" );
		return;
	}


	OGLockMutex( e->KEEPmutex );
	int hashval = case_matters_djb_hashl( hebuff ) % SPREADGINE_CACHEMAP_SIZE;
	int hashid = HashIDFromName( hebuff );
	struct SpreadHashEntry ** hashbin = &e->KEEPhash[hashid][hashval];
	while( hashbin )
	{
		if( strcmp( (*hashbin)->key, he ) == 0 )
		{
			struct SpreadHashEntry * next = (*hashbin)->next;
			int i;
			for( i = (*hashbin)->entry_in_KEEPlist; i < e->KEEPlistnum[hashid]-1; i++ )
			{
				e->KEEPlist[hashid][i] = e->KEEPlist[hashid][i+1];
			}

			free( (*hashbin)->key );
			if( (*hashbin)->payload ) free( (*hashbin)->payload );
			free( *hashbin );
			*hashbin = next;
			OGUnlockMutex( e->KEEPmutex );

			return;
		}
		hashbin = &((*hashbin)->next);
	}
	OGUnlockMutex( e->KEEPmutex );

	return;
}

int SpreadCreateDump( Spreadgine * spr, uint8_t ** ptrout )
{
	uint8_t * pout = malloc(1024);
	int poutreserved = 1024;
	int poutsize = 0;
	int i;
	int hashid = 0;
	for( ; hashid < 3; hashid++ )
	{
		for( i = 0; i < spr->KEEPlistnum[hashid]; i++ )
		{
			SpreadHashEntry * ke = spr->KEEPlist[hashid][i];
			printf( "KEY: %s\n",ke->key );
			OGLockMutex( spr->KEEPmutex );

			if( !ke ) continue;

			if( poutsize + ke->payloadlen > poutreserved )
			{
				poutreserved = poutsize + ke->payloadlen + 1024;
				pout = realloc( pout, poutreserved );
			}
			memcpy( pout + poutsize, ke->payload, ke->payloadlen );
			poutsize += ke->payloadlen;

			OGUnlockMutex( spr->KEEPmutex );
		}
	}
	*ptrout = pout;
	return poutsize;
}


