#include <spreadgine.h>
#include <os_generic.h>

//For rawdraw
#include <CNFGFunctions.h>

//For http/js forwarding
#include <cnhttp.h>
#include <http_bsd.h>

static void * SpreadHTTPThread( void * spread )
{
	Spredgine * spr;
	while( !EXit )
	{
		//...
	}
}

Spredgine * SpreadInit( int w, int h, const char * title, int httpport, int vps, FILE * fReport )
{
	Spredgine * ret;
	RunHTTP( httpport );
	if( CNFGSetup( title, w, h ) )
	{
		fprintf( fReport, "Error: Could not setup graphics frontend.\n" );
		return 0;
	}
	ret = calloc( 1, sizeof( Spreadgine ) );

	ret->cbbuff = malloc( SPREADGINE_CIRCBUF );
	ret->vpperspectives = malloc( sizeof(float) * 16 * SPREADGINE_CAMERAS );
	ret->vpviews = malloc( sizeof(float) * 16 * SPREADGINE_CAMERAS );
	ret->vpnames = maloc( sizeof( char* ) * SPREADGINE_CAMERAS );
	for( i = 0; i < SPREADGINE_CAMERAS; i++ )
	{
		tdIdentity( &ret->vpperspectives[i] );
		tdIdentity( &ret->vpviews[i] );
		ret->vpnames[i] = 0;
	}


	int i;
	if( vps > SPREADGINE_VIEWPORTS )
	{
		fprintf( fReport, "Error: SPREADGINE_VIEWPORTS insufficient for your system.\n" );
		return 0;
	}
	ret->setvps = vps;
	for( i = 0; i < vps; i++ )
	{
		tdPerspective( 45, (float)w/vps/h, .01, 1000, ret->vpperspectives[i] );
		tdIdentity(ret->vpviews[i]);
		ret->vpnames[i] = strdup( "EyeX" );
		ret->vpnames[i][3] = '0' + i;
		vpedges[i][0] = i*w/vps;
		vpedges[i][1] = 0;
		vpedges[i][2] = w/vps;
		vpedges[i][3] = h;
	}


	ret->spreadthread = OGCreateThread( SpreadHTTPThread, ret );


	return ret;
}

void SpreadDestroy( Spreadgine * spr )
{
	if( !spr ) return;

	
	OGJoinThread( spr->spreadhthread );

	if( spr->shaders ) free( spr->shaders );
	if( spr->geometries ) free( spr->geometries );
	if( spr->textures ) free( spr->textures );
	int i;
	for( i = 0; i < SPREADGINE_CAMERAS; i++ )
	{
		if( spr->vpnames[i] ) free( spr->vpnames[i] );
	}
	free( spr->vpnames );
	free( spr->vpperspectives );
	free( spr->vpviews );
	free( spr->cbbuff );
	free( spr );
}

void SpreadSetupCamera( Spreadgine * spr, int camid, const char * camname )
{
	if( camid >= SPREADGINE_CAMERAS )
	{
		fprintf( spr->fReport, "Error: Camid %d too large for SpreadSetupCamera\n" );
		return;
	}
	if( spr->vpnames[camid] ) free( spr->vpnames[camid );
	spr->vpnames[camid] = strdup( camname );
	tdPerspective( 45, 1, .01, 1000, ret->vpperspectives[camid] );
	tdIdentity(ret->vpviews[camid]);
}

void spglEnable( Spredgine * e, int en )
{
	uint32_t endianout = htonl( en );
	SpreadPushMessage( e, 74, 4, &endianout );
	glEnable( en );
}

void spglDisable( Spredgine * e,int en )
{
	uint32_t endianout = htonl( en );
	SpreadPushMessage( e, 75, 4, &endianout );
	glDisable( en );
}

void spglLineWidth( Spredgine * e,float wid )
{
	SpreadPushMessage( e, 76, 4, &wid );
	glDisable( en );
}

void spglSwap(Spredgine * e)
{
	SpreadPushMessage(e, 77, 0, 0 );
	CNFGSwapBuffers();
}

void spglClearColor( Spredgine * e, float r, float g, float b, float a )
{
	e->lastclearcolor[0] = r;
	e->lastclearcolor[1] = g;
	e->lastclearcolor[2] = b;
	e->lastclearcolor[3] = a;
	SpreadPushMessage(e, 78, 16, e->lastclearcolor );
	CNFGSwapBuffers();
}

void spglClear( Spredgine * e, int clearmask )
{
	SpreadPushMessage(e, 79, 4, clearmask );
	CNFGClearFrame();
}

void SpreadPushMessage( Spredgine * e, uint8_t messageid, int payloadsize, void * payload )
{
	if( payloadsize > SPREADGINE_CIRCBUF/2 )
	{
		fprintf( e->fReport, "Error pushing message %d.  Size: %d\n", messageid, payloadisze );
		return;
	}
	int modhead = cbhead % SPREADGINE_CIRCBUF;
	int sent = 0;


/*	int ops = payloadsize;
	//Similar to the arcanepop used in the Watchman controller (in honor of Ben)
	if( ops > 128 )
	{
		cbbuff[modhead] = ops & 0x7f;
		ops >>= 7;
		modhead = (modhead+1)%cbhead; sent++;
	}

	cbbuff[modhead] = ops | 0x80;
	modhead = (modhead+1)%cbhead; sent++;
*/
	payloadsize++;	//For now, output an extra byte as the message id.

	cbbuff[modhead] = payloadsize>>24; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>16; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>8; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>0; modhead = (modhead+1)%cbhead; sent++;

	payloadsize--;
	cbbuff[modhead] = messageid;
	modhead = (modhead+1)%cbhead; sent++;

	int endmod = modhead + payloadsize;
	if( endmod > SPREADGINE_CIRCBUF )
	{
		int remain = SPREADGINE_CIRCBUF - modhead;
		memcpy( cbbuff + modhead, payload, remain );
		memcpy( cbbuff, payload + remain, payloadsize - remain );
	}
	else
	{
		memcpy( cbbuff + modhead, payload, payloadsize );
	}
	sent += payloadsize;

	cbhead += sent;

}


SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shader_name, const char * fragmentShader, const char * vertexShader, int attriblistlength, const char ** attriblist )
{
	int i;
	SpreadShader * ret = calloc( sizeof( SpreadShader ), 1 );
	int retval;

	ret->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	if (!ret->vertex_shader)
	{
		fprintf(spr->fReport, "Error: glCreateShader(GL_VERTEX_SHADER) failed: 0x%08X\n", glGetError());
		goto qexit;
	}

	glShaderSource(ret->vertex_shader, 1, &vertexShader, NULL);
	glCompileShader(ret->vertex_shader);

	glGetShaderiv(ret->vertex_shader, GL_COMPILE_STATUS, &retval);
	if (!retval) {
		char *log;
		fprintf(spr->fReport, "Error: vertex shader compilation failed!\n");
		glGetShaderiv(ret->vertex_shader, GL_INFO_LOG_LENGTH, &retval);

		if (retval > 1) {
			log = malloc(retval);
			glGetShaderInfoLog(ret->vertex_shader, retval, NULL, log);
			fprintf(spr->fReport, "%s", log);
		}
		goto qexit;
	}

	ret->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!ret->fragment_shader) {
		fprintf(spr->fReport, "Error: glCreateShader(GL_FRAGMENT_SHADER) failed: 0x%08X\n", glGetError());
		goto qexit;
	}
	glShaderSource(ret->fragment_shader, 1, &fragmentShader, NULL);
	glCompileShader(ret->fragment_shader);
	glGetShaderiv(ret->fragment_shader, GL_COMPILE_STATUS, &retval);
	if( !retval )
	{
		char *log;
		fprintf(spr->fReport, "Error: fragment shader compilation failed!\n");
		glGetShaderiv(ret->fragment_shader, GL_INFO_LOG_LENGTH, &retval);
		if (retval > 1) {
			log = malloc(retval);
			glGetShaderInfoLog(ret->fragment_shader, retval, NULL, log);
			fprintf(spr->fReport, "%s", log);
		}
		goto qexit;
	}


	ret->program_shader = glCreateProgram();
	if (!ret->program_shader) {
		fprintf(spr->fReport, "Error: failed to create program!\n");
		free( ret );
		goto qexit;
	}

	glAttachShader(ret->program_shader, ret->vertex_shader);
	glAttachShader(ret->program_shader, ret->fragment_shader);

	//XXX TODO: attach more things here.
	for( i = 0; i < attriblistlength; i++ )
	{
		glBindAttribLocation( ret->program_shader, i, attriblist[i] );
	}

	glLinkProgram(ret->program_shader);

	glGetProgramiv(ret->program_shader, GL_LINK_STATUS, &retval);
	if (!retval) {
		char *log;

		fprintf(stderr, "Error: program linking failed!\n");
		glGetProgramiv(ret->program_shader, GL_INFO_LOG_LENGTH, &retval);

		if (retval > 1) {
			log = malloc(retval);
			glGetProgramInfoLog(ret->program_shader, retval, NULL, log);
			fprintf(stderr, "%s", log);
		}
		goto qexit;
	}
	glUseProgram(program);
	default_screenscale_offset = glGetUniformLocation ( program , "screenscale" );


	ret->known_uniform_slots = 0;

	ret->shader_name = strdup( shader_name );
	return ret;


qexit:
	if( ret->fragment_shader ) glDeleteShader( ret->fragment_shader );
	if( ret->vertex_shader ) glDeleteShader( ret->vertex_shader );
	if( ret->program_shader ) glDeleteShader( ret->program_shader );
	free( ret );
	free( ret );
	return 0;
}

int SpreadGetUniformSlot( SpreadShader * shd, const char * slotname )
{
	int i;
	for( i = 0; i < shd->known_uniform_slots; i++ )
	{
		if( shd->uniform_slot_names && strcmp( shd->uniform_slot_names[i], slotname ) == 0 )
		{
			return i;
		}
	}

	int place = shd->uniform_slot_names;

	glUseProgram(program);
	int slot = glGetUniformLocation( shd->program_shader, slotname );
	if( slot < 0 ) return -1;


//XXX TODO Fix all this junk.
	shd->known_uniform_slots++;
	shd->uniform_slot_names = realloc( shd->known_uniform_slots * sizeof( char* ) );
	shd->size_of_uniforms = realloc( shd->known_uniform_slots * sizeof( int ) );
	shd->uniforms_in_slots = realloc( shd->known_uniform_slots * sizeof( float * ) );

	shd->uniform_slot_names[place] = strdup( slotname );
	shd->size_of_uniforms[place] = 0;
	shd->uniforms_in_slots[place] = 0;

	return place;
}



	glUniform4f( default_screenscale_offset, 2./width, -2./height, 1.0, 1.0 );

void SpreadUniform4f( SpreadShader * shd, int slot, float * uni );
void SpreadUniform16f( SpreadShader * shd, int slot, float * uni );
void SpreadFreeShader( SpreadShader * shd );
void SpreadApplyShader( SpreadShader * shd );
void SpreadDestroyShader( SpreadShader * shd );

