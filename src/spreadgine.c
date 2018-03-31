//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

#include <string.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>
#include <os_generic.h>
#include <arpa/inet.h> //For htonl

//For rawdraw
#include <CNFGFunctions.h>
#include <CNFG3D.h>




Spreadgine * SpreadInit( int w, int h, const char * title, int httpport, int vps, FILE * fReport )
{
	int i;
	Spreadgine * ret;

	if( CNFGSetup( title, w, h ) )
	{
		fprintf( fReport, "Error: Could not setup graphics frontend.\n" );
		return 0;
	}
	ret = calloc( 1, sizeof( Spreadgine ) );
	ret->fReport = fReport;
	ret->cbbuff = malloc( SPREADGINE_CIRCBUF );
	ret->vpperspectives = malloc( sizeof(float) * 16 * SPREADGINE_CAMERAS );
	ret->vpviews = malloc( sizeof(float) * 16 * SPREADGINE_CAMERAS );
	ret->vpnames = malloc( sizeof( char* ) * SPREADGINE_CAMERAS );


	for( i = 0; i < SPREADGINE_CAMERAS; i++ )
	{
		tdIdentity( ret->vpperspectives[i] );
		tdIdentity( ret->vpviews[i] );
		ret->vpnames[i] = 0;
	}

	if( vps > SPREADGINE_VIEWPORTS )
	{
		fprintf( fReport, "Error: SPREADGINE_VIEWPORTS insufficient for your system.\n" );
		return 0;
	}

	SpreadRemoteInit( ret );

	ret->setvps = vps;
	for( i = 0; i < vps; i++ )
	{
	//XXX TODO: Update with
		char EyeName[5] = { 'E', 'y', 'e', '0'+i };
		SpreadSetupCamera( ret, i, 75, (float)w/vps/h, .01, 1000, EyeName );
		tdIdentity(ret->vpviews[i]);
		ret->vpnames[i] = strdup( "EyeX" );
		ret->vpnames[i][3] = '0' + i;
		ret->vpedges[i][0] = i*w/vps;
		ret->vpedges[i][1] = 0;
		ret->vpedges[i][2] = w/vps;
		ret->vpedges[i][3] = h;
	}

	{
		//First: Add a defualt shader
		static const char *default_vertex_shader_source =
			"attribute vec3 vpos;\n"
			"attribute vec4 vcolor;\n"
			"uniform mat4 pmatrix, vmatrix, mmatrix;\n"
			"varying vec4 vvColor;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    vvColor = vcolor;\n"
			"    gl_Position = (pmatrix * (vmatrix * (mmatrix * vec4(vpos, 1.0)))); \n"
			"}\n";
		static const char *default_fragment_shader_source =
			"precision mediump float;\n"
			"\n"
			"varying vec4 vvColor;\n"
			"\n"
			"void main()\n"
			"{\n"
			"    gl_FragColor = vec4( vvColor.rgb, 1.0 );\n"
			"}\n";
	
		const char * attribos[2] = { "vpos", "vcolor" };
		SpreadShader * shd0 = SpreadLoadShader( ret, "shd0", default_fragment_shader_source, default_vertex_shader_source, 2, attribos );
		if( !shd0 )
		{
			fprintf( fReport, "Error making shader.\n" );
		}
	}

	{
		static const float CubeDataVerts[36*3] = {
			-1.0f,-1.0f,-1.0f,	-1.0f,-1.0f, 1.0f,	-1.0f, 1.0f, 1.0f,
			-1.0f,-1.0f,-1.0f,	-1.0f, 1.0f, 1.0f,	-1.0f, 1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,	1.0f,-1.0f,-1.0f,	1.0f, 1.0f,-1.0f,
			1.0f,-1.0f,-1.0f,	1.0f, 1.0f, 1.0f,	1.0f,-1.0f, 1.0f,

			1.0f, 1.0f,-1.0f,	-1.0f,-1.0f,-1.0f,	-1.0f, 1.0f,-1.0f,
			1.0f, 1.0f,-1.0f,	1.0f,-1.0f,-1.0f,	-1.0f,-1.0f,-1.0f,
			-1.0f, 1.0f, 1.0f,	-1.0f,-1.0f, 1.0f,	1.0f,-1.0f, 1.0f,
			1.0f, 1.0f, 1.0f,	-1.0f, 1.0f, 1.0f,	1.0f,-1.0f, 1.0f,

			1.0f,-1.0f, 1.0f,	-1.0f,-1.0f,-1.0f,	1.0f,-1.0f,-1.0f,
			1.0f,-1.0f, 1.0f,	-1.0f,-1.0f, 1.0f,	-1.0f,-1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,	1.0f, 1.0f,-1.0f,	-1.0f, 1.0f,-1.0f,
			1.0f, 1.0f, 1.0f,	-1.0f, 1.0f,-1.0f,	-1.0f, 1.0f, 1.0f,
		};

		static const float CubeColorData[36*4] = {
			1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,
			0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,
			0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,
			0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,
			1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,
			1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,
		};

		static int strides[2] = { 3, 4 };
		static int types[2] = { GL_FLOAT, GL_FLOAT };
		static int typesizes[2] = { 4, 4 };
		const float * arrays[] = { CubeDataVerts, CubeColorData };

		SpreadGeometry * geo0 = SpreadCreateGeometry( ret, "geo1", GL_TRIANGLES, 36, 2, (const void **)arrays, strides, types, typesizes  );
		if( !geo0 )
		{
			fprintf( fReport, "Error making geometry.\n" );
		}
	}

	return ret;
}


void SpreadDestroy( Spreadgine * spr )
{
	if( !spr ) return;

	//XXX TODO : Kill net thread
	
	OGJoinThread( spr->spreadthread );

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

void SpreadSetupCamera( Spreadgine * spr, uint8_t camid, float fov, float aspect, float near, float far, const char * camname )
{
	if( camid >= SPREADGINE_CAMERAS )
	{
		fprintf( spr->fReport, "Error: Camid %d too large for SpreadSetupCamera\n", camid );
		return;
	}
	if( spr->vpnames[camid] ) free( spr->vpnames[camid] );
	spr->vpnames[camid] = strdup( camname );
	tdPerspective( 45, 1, .01, 1000, spr->vpperspectives[camid] );
	tdIdentity(spr->vpviews[camid]);

	//XXX TODO ADD Update to network.
//	( spr, camid );
//	SpreadBumpConfiguration( spr );
}

void spglEnable( Spreadgine * e, uint32_t en )
{
	uint32_t endianout = htonl( en );
	SpreadPushMessage( e, 74, 4, &endianout );
	glEnable( en );
}

void spglDisable( Spreadgine * e,uint32_t de )
{
	uint32_t endianout = htonl( de );
	SpreadPushMessage( e, 75, 4, &endianout );
	glDisable( de );
}

void spglLineWidth( Spreadgine * e,float wid )
{
	SpreadPushMessage( e, 76, 4, &wid );
	glLineWidth( wid );
}

void spglSwap(Spreadgine * e)
{
	SpreadPushMessage(e, 77, 0, 0 );
	CNFGSwapBuffers();
}

void spglClearColor( Spreadgine * e, float r, float g, float b, float a )
{
	e->lastclearcolor[0] = r;
	e->lastclearcolor[1] = g;
	e->lastclearcolor[2] = b;
	e->lastclearcolor[3] = a;
	SpreadPushMessage(e, 78, 16, e->lastclearcolor );
	glClearColor( r, g, b, a );
}

void spglClear( Spreadgine * e, uint32_t clearmask )
{
	SpreadPushMessage(e, 79, 4, &clearmask );
	glClear( clearmask );
}


SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shadername, const char * fragmentShader, const char * vertexShader, int attriblistlength, const char ** attriblist )
{
	int i;
	int shaderindex = 0;
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
	ret->parent = spr;

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

	ret->model_index = glGetUniformLocation( ret->program_shader, "mmatrix" );
	ret->view_index = glGetUniformLocation( ret->program_shader, "vmatrix" );
	ret->perspective_index = glGetUniformLocation( ret->program_shader, "pmatrix" );

	glUseProgram(ret->program_shader);

	spr->current_shader = ret->shader_in_parent;

	SpreadMessage( spr, "shader%d", "sssiS", ret->shader_in_parent, shadername, fragmentShader, vertexShader, attriblistlength, attriblistlength, attriblist );
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
	if( slot > shd->known_uniform_slots )
	{
		shd->known_uniform_slots = slot+1;
		shd->uniform_slot_names = realloc( shd->uniform_slot_names, shd->known_uniform_slots * sizeof( char* ) );
		shd->uniform_slot_name_lens = realloc( shd->uniform_slot_name_lens, shd->known_uniform_slots * sizeof( int ) );
		shd->size_of_uniforms = realloc( shd->size_of_uniforms, shd->known_uniform_slots * sizeof( int ) );
		shd->uniforms_in_slots = realloc( shd->uniforms_in_slots, shd->known_uniform_slots * sizeof( float * ) );

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
	SpreadPushMessage(shd->parent, 81, sizeof(float)*4+stlen, outputarray );
}

void SpreadUniform16f( SpreadShader * shd, int slot, const float * uni )
{
	int stlen = shd->uniform_slot_name_lens[slot];
	char outputarray[stlen+sizeof(float)*16];
	glUniformMatrix4fv( slot, 1, 1, uni );
	memcpy( outputarray, uni, sizeof(float)*16 );
	memcpy( outputarray + sizeof(float)*16, shd->uniform_slot_names[slot], stlen );
	SpreadPushMessage(shd->parent, 82, sizeof(float)*16+stlen, outputarray );
}

void SpreadApplyShader( SpreadShader * shd )
{
	glUseProgram(shd->program_shader);
	SpreadPushMessage(shd->parent, 80, 4, &shd->shader_in_parent );
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

//	SpreadBumpConfiguration( shd->parent );
}





SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, const char * geoname, int render_type, int verts, int nr_arrays, const void ** arrays, int * strides, int * types, int * typesizes )
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
	ret->parent = spr;
	ret->numarrays = nr_arrays;
	ret->arrays = malloc( sizeof(void*) * nr_arrays );
	ret->types = malloc( sizeof(int) * nr_arrays );
	ret->strides = malloc( sizeof(int) * nr_arrays );
	ret->typesizes = malloc( sizeof(int) * nr_arrays );

	for( i = 0; i < nr_arrays; i++ )
	{
		ret->types[i] = types[i];
		int stride = ret->strides[i] = strides[i];
		int typesize = ret->typesizes[i] = typesizes[i];
		ret->arrays[i] = malloc( stride * typesize * verts );
		memcpy( ret->arrays[i], arrays[i], stride * typesize * verts );
	}


	//SpreadBumpConfiguration( ret->parent );

	return ret;
}

void UpdateSpreadGeometry( SpreadGeometry * geo, int arrayno, void * arraydata )
{
	int arraysize = geo->strides[arrayno] * geo->typesizes[arrayno] * geo->verts;
	uint8_t trray[arraysize + 8] __attribute__((aligned(32)));
	memcpy( geo->arrays[arrayno], arraydata, arraysize );
	((uint32_t*)trray)[0] = htonl( geo->geo_in_parent );
	((uint32_t*)trray)[1] = htonl( arrayno );
	memcpy( trray+8, arraydata, arraysize );
	SpreadPushMessage(geo->parent, 88, arraysize+8, trray );
}

void SpreadRenderGeometry( SpreadGeometry * geo, int start, int nr_emit, const float * modelmatrix )
{
	Spreadgine * parent = geo->parent;
	SpreadShader * ss = &parent->shaders[parent->current_shader];
	int vmatpos = ss->view_index;
	int pmatpos = ss->perspective_index;
	int mmatpos = ss->model_index;

	glUniformMatrix4fv( mmatpos, 1, 1, modelmatrix );

	int i;
	for( i = 0; i < geo->numarrays; i++ )
	{
		glVertexAttribPointer( i, geo->strides[i], geo->types[i], GL_FALSE, 0, geo->arrays[i] );
	    glEnableVertexAttribArray(i);
	}

	for( i = 0; i < parent->setvps; i++ )
	{
		int * vpedges = parent->vpedges[i];
		glUniformMatrix4fv( vmatpos, 1, 1, parent->vpviews[i] );
		glUniformMatrix4fv( pmatpos, 1, 1, parent->vpperspectives[i] );
		glViewport(vpedges[0],  vpedges[1], vpedges[2], vpedges[3]); 

		glDrawArrays(geo->render_type, start, nr_emit);
	}

	uint32_t SpreadGeoInfo[3+16];
	SpreadGeoInfo[0] = htonl( geo->geo_in_parent );
	SpreadGeoInfo[1] = htonl( start );
	SpreadGeoInfo[2] = htonl( nr_emit );
	memcpy( &SpreadGeoInfo[3], modelmatrix, sizeof( float ) * 16 );
	SpreadPushMessage(geo->parent, 89, sizeof(SpreadGeoInfo), SpreadGeoInfo );
}



