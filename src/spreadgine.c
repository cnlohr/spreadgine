//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

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
	while( 1 )
	{
		//DO WORK HERE
		//Also figure out a graceful way of quitting when spreadgine wants to shut down.
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
	if( spr->geos ) free( spr->geos );
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

	SpreadBumpConfiguration( spr );
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

	cbbuff[modhead] = payloadsize>>24; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>16; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>8; modhead = (modhead+1)%cbhead; sent++;
	cbbuff[modhead] = payloadsize>>0; modhead = (modhead+1)%cbhead; sent++;

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


SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shadername, const char * fragmentShader, const char * vertexShader, int attriblistlength, const char ** attriblist )
{
	int i;
	int retval;
	SpreadShader * ret;

	//First see if there are any free shaders available in the parent...
	for( i = 0; i < spr->setshaders; i++ )
	{
		if( spr->shaders[i].shadername == 0 )
			break;
	}
	if( i == spr->setshaders )
	{
		spr->shaders = realloc( spr->shaders, (spr->setshaders+1)* sizeof( SpreadShader ) );
		ret = &spr->shaders[spr->setshaders];
		memset( ret, 0, sizeof( SpreadShader ) );
	}
	else
	{
		ret = &spr->shaders[i];
	}

	ret->shader_in_parent = i;

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

	ret->known_uniform_slots = 0;
	ret->shadername = strdup( shadername );
	ret->fragment_shader_text = strdup( fragmentShader );
	ret->vertex_shader_text = strdup( vertexShader );

	spr->setshaders++;

	ret->view_index = glGetUniformLocation( ret->program_shader, "vmatrix" );
	ret->perspective_index = glGetUniformLocation( ret->program_shader, "pmatrix" );

	glUseProgram(ret->program_shader);

	spr->current_shader = ret->shader_in_parent;

	SpreadBumpConfiguration( spr );

	return ret;


qexit:
	if( ret->fragment_shader ) glDeleteShader( ret->fragment_shader );
	if( ret->vertex_shader ) glDeleteShader( ret->vertex_shader );
	if( ret->program_shader ) glDeleteShader( ret->program_shader );
	return 0;
}

int SpreadGetUniformSlot( SpreadShader * shd, const char * slotname )
{
	int i;
	int slot = glGetUniformLocation( shd->program_shader, slotname );
	if( slot > known_uniform_slots )
	{
		shd->known_uniform_slots = slot+1;
		shd->uniform_slot_names = realloc( shd->known_uniform_slots * sizeof( char* ) );
		shd->uniform_slot_name_lens = realloc( shd->uniform_slot_name_lens * sizeof( int ) );
		shd->size_of_uniforms = realloc( shd->known_uniform_slots * sizeof( int ) );
		shd->uniforms_in_slots = realloc( shd->known_uniform_slots * sizeof( float * ) );

		shd->uniform_slot_names[slot] = strdup( slotname );
		shd->uniform_slot_name_lens[slot] = strlen( slotname );
		shd->size_of_uniforms[slot] = 0;
		shd->uniforms_in_slots[slot] = 0;
	}

	return slot;
}

void SpreadUniform4f( SpreadShader * shd, int slot, const float * uni )
{
	int stlen = shd->uniform_slot_name_lens[slot];
	char outputarray[stlen+sizeof(float)*4];
	glUniform4fv( slot, 1, uni );
	memcpy( outputarray, uni, sizeof(float)*4 );
	memcpy( outputarray + sizeof(float)*4, shd->uniform_slot_names[slot], stlen );
	SpreadPushMessage(e, 81, sizeof(float)*4+stlen, outputarray );
}

void SpreadUniform16f( SpreadShader * shd, int slot, const float * uni )
{
	int stlen = shd->uniform_slot_name_lens[slot];
	char outputarray[stlen+sizeof(float)*16];
	glUniformMatrix4fv( slot, 1, uni );
	memcpy( outputarray, uni, sizeof(float)*16 );
	memcpy( outputarray + sizeof(float)*16, shd->uniform_slot_names[slot], stlen );
	SpreadPushMessage(e, 82, sizeof(float)*16+stlen, outputarray );
}

void SpreadApplyShader( SpreadShader * shd )
{
	glUseProgram(shd->program_shader);
	SpreadPushMessage(e, 80, 4, &shd->shader_in_parent );
}

void SpreadFreeShader( SpreadShader * shd )
{
	if( shd->fragment_shader ) glDeleteShader( shd->fragment_shader );
	if( shd->vertex_shader ) glDeleteShader( shd->vertex_shader );
	if( shd->program_shader ) glDeleteShader( shd->program_shader );
	if( shd->uniform_slot_names ) 		{ free( shd->uniform_slot_names ) ;  	shd->uniform_slot_names = 0; }
	if( shd->uniform_slot_name_lens ) 	{ free( shd->uniform_slot_name_lens ); 	shd->uniform_slot_name_lens = 0; }
	if( shd->size_of_uniforms ) 		{ free( shd->size_of_uniforms ); 		shd->size_of_uniforms = 0; }
	if( shd->uniforms_in_slots ) 		{ free( shd->uniforms_in_slots );		shd->uniforms_in_slots = 0; }
	if( shd->shadername )				{ free( shd->shadername );				shd->shadername = 0; }
	if( shd->fragment_shader_text )		{ free( shd->fragment_shader_text );	shd->fragment_shader_text = 0; }
	if( shd->vertex_shader_text )		{ free( shd->vertex_shader_text );		shd->vertex_shader_text = 0; }

	SpreadBumpConfiguration( shd->parent );
}





SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, const char * geoname, int render_type, int verts, int nr_arrays, void ** arrays, int * strides, int * types, int * typesizes )
{

	int i;
	int retval;
	SpreadGeometry * ret;

	//First see if there are any free geos available in the parent...
	for( i = 0; i < spr->setgeos; i++ )
	{
		if( spr->geos[i].geoname == 0 )
			break;
	}
	if( i == spr->setgeos )
	{
		spr->geos = realloc( spr->geos, (spr->setgeos+1)* sizeof( SpreadGeometry ) );
		ret = &spr->geos[spr->setgeos];
		memset( ret, 0, sizeof( SpreadGeometry ) );
	}
	else
	{
		ret = &spr->geos[i];
	}

	ret->geo_in_parent = i;
	ret->geoname = strdup( geoname );
	ret->render_type = render_type;
	ret->verts = verts;
	ret->numarrays = nr_arrays;
	ret->arrays = malloc( sizeof(void*) * nr_arrays );
	ret->types = malloc( sizeof(int) * nr_arrays );
	ret->strides = malloc( sizeof(int) * nr_arrays );
	ret->typesizes = malloc( sizeof(int) * nr_arrays );

	for( i = 0; i < nr_arrays; i++ )
	{
		ret->types[i] = typestobu[i];
		int stride = ret->strides[i] = strides[i];
		int typesize = ret->typesizes[i] = typesizes[i];
		ret->arrays[i] = malloc( stride * typesize * verts );
		memcpy( ret->arrays[i], arrays[i], stride * typesize * verts );
	}


	SpreadBumpConfiguration( shd->parent );
}

void UpdateSpreadGeometry( SpreadGeometry * geo, int arrayno, void * arraydata )
{
	int arraysize = geo->strides[arrayno] * geo->typesizes[arrayno] * geo->verts;
	uint8_t trray[arraysize + 8] (__attribute__(aligned(32)));
	memcpy( geo->arrays[arrayno], arraydata, arraysize );
	((uint32_t*)trray)[0] = htonl( geo->geo_in_parent );
	((uint32_t*)trray)[1] = htonl( arrayno );
	memcpy( trray+8, arraydata, arraysize );
	SpreadPushMessage(geo->parent, 88, arraysize+8, trray );
}

void SpreadRenderGeometry( SpreadGeometry * geo, int start, int nr_emit )
{
	Spreadgine * parent = geo->parent;
	int vmatpos = parent->shaders[parent->current_shader].view_index;
	int pmatpos = parent->shaders[parent->current_shader].perspective_index;

	int i;
	for( i = 0; i < geo->numarrays; i++ )
	{
		glVertexAttribPointer( i, geo->strides[i], geo->types[i], GL_FALSE, 0, geo->arrays[i] );
	    glEnableVertexAttribArray(i);
	}

	for( i = 0; i < parent->setvps; i++ )
	{
		glUniformMatrix4fv( vmatpos, 1, parent->vpviews[i] );
		glUniformMatrix4fv( pmatpos, 1, parent->vpperspectives[i] );
		glViewport(vpedges[i][0],  vpedges[i][1], vpedges[i][2], vpedges[i][2]); 

		glDrawArrays(geo->render_type, start, nr_emit);
	}

	uint32_t SpreadGeoInfo[3];
	SpreadGeoInfo[0] = htonl( geo->geo_in_parent );
	SpreadGeoInfo[1] = htonl( start );
	SpreadGeoInfo[2] = htonl( nr_emit );
	SpreadPushMessage(geo->parent, 89, sizeof(SpreadGeoInfo), SpreadGeoInfo );
}



