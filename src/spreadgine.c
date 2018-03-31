//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

#include <string.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>
#include <os_generic.h>
#include <arpa/inet.h> //For htonl
#include <stdlib.h>

//For rawdraw
#include <CNFGFunctions.h>
#include <CNFG3D.h>


uint8_t SpreadTypeSizes[] = { 4, 1 };

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
	SpreadMessage( ret, "setup", "biiis", 64, w, h, vps, title );


	ret->setvps = vps;
	for( i = 0; i < vps; i++ )
	{
	//XXX TODO: Update with
		char EyeName[5] = { 'E', 'y', 'e', '0'+i };
		SpreadSetupCamera( ret, i, 75, (float)w/vps/h, .01, 1000, EyeName );
		tdIdentity(ret->vpviews[i]);
		SpreadChangeCameaView( ret, i, ret->vpviews[i] );


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
			"#ifdef GL_ES\n"
			"precision mediump float;\n"
			"#endif\n"
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
		#define SIMPLECUBE
		#ifdef SIMPLECUBE
		/* init_resources */
		uint16_t CubeDataIndices[] = {
			0, 1, 2,	2, 3, 0,	// front
			1, 5, 6,	6, 2, 1,	// right
			7, 6, 5,	5, 4, 7,	// back
			4, 0, 3,	3, 7, 4,	// left
			4, 5, 1,	1, 0, 4,	// bottom
			3, 2, 6,	6, 7, 3,	// top
		}; 		int IndexQty = 36;

		static const float CubeDataVerts[] = {
			-1.0, -1.0,  1.0,	 1.0, -1.0,  1.0,	 1.0,  1.0,  1.0,	-1.0,  1.0,  1.0,			// front
			-1.0, -1.0, -1.0,	 1.0, -1.0, -1.0,	 1.0,  1.0, -1.0,	-1.0,  1.0, -1.0,			// back
		};		int VertQty = 8;

		static const float CubeDataColors[] = {
			1.0, 0.0, 0.0, 1.0,		0.0, 1.0, 0.0, 1.0,		0.0, 0.0, 1.0, 1.0,		1.0, 1.0, 1.0, 1.0,			// front colors
			1.0, 0.0, 0.0, 1.0,		0.0, 1.0, 0.0, 1.0,		0.0, 0.0, 1.0, 1.0,		1.0, 1.0, 1.0, 1.0,			// back colors
		};

		#else
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
		};	int VertQty = 36;

		static const float CubeDataColors[36*4] = {
			1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,	1.,0.,0.,1.,
			0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,	0.,1.,0.,1.,
			0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,	0.,0.,1.,1.,
			0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,	0.,1.,1.,1.,
			1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,	1.,0.,1.,1.,
			1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,	1.,1.,0.,1.,
		};

		static uint16_t CubeDataIndices[36];
		for( i = 0; i < 36; i++ ) CubeDataIndices[i] = i;
		int IndexQty = 36;

		#endif

		static int strides[2] = { 3, 4 };
		static int types[2] = { GL_FLOAT, GL_FLOAT };
		const float * arrays[] = { CubeDataVerts, CubeDataColors };

		SpreadGeometry * geo0 = SpreadCreateGeometry( ret, "geo1", GL_TRIANGLES, IndexQty, CubeDataIndices, VertQty, 2, (const void **)arrays, strides, types  );
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

	spr->doexit = 1;
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
	tdPerspective( fov, aspect, near, far, spr->vpperspectives[camid] );
	tdIdentity(spr->vpviews[camid]);

	SpreadMessage( spr, "camera#", "bbffffs", camid, 68, camid, fov, aspect, near, far, camname );
	SpreadChangeCameaPerspective( spr, camid, spr->vpperspectives[camid]  );
}

void SpreadChangeCameaPerspective( Spreadgine * spr, uint8_t camid, float * newpersp )
{
	memcpy( spr->vpperspectives[camid], newpersp, sizeof(float)*16 );
	SpreadMessage( spr, "campersp#", "bbX", camid, 66, camid, 16*sizeof(float), newpersp );
}

void SpreadChangeCameaView( Spreadgine * spr, uint8_t camid, float * newview )
{
	memcpy( spr->vpviews[camid], newview, sizeof(float)*16 );
	SpreadMessage( spr, "camview#", "bbX", camid, 67, camid, 16*sizeof(float), newview );
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
	uint32_t lcmask = htonl(clearmask);
	SpreadPushMessage(e, 79, 4, &lcmask );
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

	for( i = 0; i < attriblistlength; i++ )
	{
		glBindAttribLocation( ret->program_shader, i, attriblist[i] );
	}

	glLinkProgram(ret->program_shader);

	glGetProgramiv(ret->program_shader, GL_LINK_STATUS, &retval);
	if (!retval) {
		char *log;

		fprintf(spr->fReport, "Error: program linking failed!\n");
		glGetProgramiv(ret->program_shader, GL_INFO_LOG_LENGTH, &retval);

		if (retval > 1) {
			log = malloc(retval);
			glGetProgramInfoLog(ret->program_shader, retval, NULL, log);
			fprintf(spr->fReport, "%s", log);
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

	int err = glGetError();
	if( err )
	{
		fprintf( spr->fReport, "Hanging error on shader compile %d (0x%02x)\n", err, err );
	}

	spr->current_shader = ret->shader_in_parent;

	SpreadMessage( spr, "shader#", "bbsssS", ret->shader_in_parent, 69, ret->shader_in_parent, shadername, fragmentShader, vertexShader, attriblistlength, attriblist );
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
	glUniformMatrix4fv( slot, 1, 0, uni );
	memcpy( outputarray, uni, sizeof(float)*16 );
	memcpy( outputarray + sizeof(float)*16, shd->uniform_slot_names[slot], stlen );
	SpreadPushMessage(shd->parent, 82, sizeof(float)*16+stlen, outputarray );
}

void SpreadApplyShader( SpreadShader * shd )
{
	glUseProgram(shd->program_shader);
	uint32_t sip = htonl( shd->shader_in_parent );
	//SpreadPushMessage(shd->parent, 80, 4, &sip );
	SpreadMessage( shd->parent, "curshader", "bi", 80, shd->shader_in_parent );
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

	SpreadMessage( shd->parent, 0, "bb", 70, shd->shader_in_parent );
	SpreadHashRemove( shd->parent, "shader#", shd->shader_in_parent );

}

SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, const char * geoname, int render_type, int indices, uint16_t * indexarray, int verts, int nr_arrays, const void ** arrays, int * strides, int * types )
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

	ret->indices = indices;
	ret->indexarray = malloc(indices*sizeof(uint16_t));
	memcpy( ret->indexarray, indexarray, sizeof(uint16_t)*indices );

	ret->geo_in_parent = i;
	ret->geoname = strdup( geoname );
	ret->render_type = render_type;
	ret->verts = verts;
	ret->parent = spr;
	ret->numarrays = nr_arrays;
	ret->arrays = malloc( sizeof(void*) * nr_arrays );
	ret->types = malloc( sizeof(uint8_t) * nr_arrays );
	ret->strides = malloc( sizeof(uint8_t) * nr_arrays );
	
	glGenBuffers(1, &ret->ibo );
 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ret->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)*indices, ret->indexarray, GL_STATIC_DRAW);
	ret->laststartv = 0;

	ret->vbos = malloc( sizeof( uint32_t ) * nr_arrays );
	glGenBuffers(nr_arrays, ret->vbos );

	for( i = 0; i < nr_arrays; i++ )
	{
		int typesize = 0;
		if( types[i] == GL_FLOAT )
		{
			ret->types[i] = 0;
		}
		else if( types[i] == GL_UNSIGNED_BYTE )
		{
			ret->types[i] = 1;
		}
		else
		{
			fprintf( spr->fReport, "Error: bad 'type' passed into SpreadCreateGeometry. Assuming GL_FLOAT.\n" );
			ret->types[i] = 0;
		}
		int stride = ret->strides[i] = strides[i];
			typesize = SpreadTypeSizes[ret->types[i]];
		ret->arrays[i] = malloc( stride * typesize * verts );
		memcpy( ret->arrays[i], arrays[i], stride * typesize * verts );

	 	glBindBuffer(GL_ARRAY_BUFFER, ret->vbos[i]);
		glBufferData(GL_ARRAY_BUFFER, stride * typesize * verts, ret->arrays[i], GL_STATIC_DRAW);

		SpreadMessage( spr, "geodata#_#", "bbbv", ret->geo_in_parent, i, 88, ret->geo_in_parent, i, stride * typesize * verts, ret->arrays[i] );
	}

	{
		uint16_t SendIndexArray[indices];
		for( i = 0; i < indices; i++ )
		{
			SendIndexArray[i] = htons( ret->indexarray[i] );
		}
		SpreadMessage( spr, "geometry#", "bbsiibvvv", ret->geo_in_parent, 87, ret->geo_in_parent, geoname, render_type, verts, nr_arrays,
			sizeof(uint8_t)*nr_arrays, ret->strides, 
			sizeof(uint8_t)*nr_arrays, ret->types,
			sizeof(uint16_t)*indices, SendIndexArray );
	}


	int err = glGetError();
	if( err )
	{
		fprintf( spr->fReport, "Hanging error on geometry compile %d (0x%02x)\n", err, err );
	}

	return ret;
}

void UpdateSpreadGeometry( SpreadGeometry * geo, int arrayno, void * arraydata )
{
	int arraysize = geo->strides[arrayno] * SpreadTypeSizes[ geo->types[arrayno] ] * geo->verts;
	SpreadMessage( geo->parent, "geodata#_#", "bbbv", geo->geo_in_parent, arrayno, 88, geo->geo_in_parent, arrayno, arraysize, arraydata );
}

void SpreadRenderGeometry( SpreadGeometry * geo, const float * modelmatrix, int startv, int numv )
{
	Spreadgine * parent = geo->parent;
	SpreadShader * ss = &parent->shaders[parent->current_shader];
	int vmatpos = ss->view_index;
	int pmatpos = ss->perspective_index;
	int mmatpos = ss->model_index;

	glUniformMatrix4fv( mmatpos, 1, 0, modelmatrix );
	//tdPrint( modelmatrix );

	int i;
	for( i = 0; i < geo->numarrays; i++ )
	{
		glBindBuffer(GL_ARRAY_BUFFER, geo->vbos[i]);
		glVertexAttribPointer( i, geo->strides[i], (geo->types[i]==0)?GL_FLOAT:GL_UNSIGNED_BYTE, GL_FALSE, 0, /*geo->arrays[i]*/0 );
	    glEnableVertexAttribArray(i);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo->ibo);
	if( startv != geo->laststartv )
	{
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t)*geo->indices-startv, geo->indexarray+startv, GL_STATIC_DRAW);
		geo->laststartv = startv;
	}

	for( i = 0; i < parent->setvps; i++ )
	{
		int * vpedges = parent->vpedges[i];
		glUniformMatrix4fv( vmatpos, 1, 0, parent->vpviews[i] );
		glUniformMatrix4fv( pmatpos, 1, 0, parent->vpperspectives[i] );
		glViewport(vpedges[0],  vpedges[1], vpedges[2], vpedges[3]); 

		glDrawElements(geo->render_type, (numv==-1)?geo->indices:numv, GL_UNSIGNED_SHORT, 0);
	}

	uint16_t SpreadGeoInfo[3+32];
	SpreadGeoInfo[0] = htons( geo->geo_in_parent );
	SpreadGeoInfo[1] = htons( startv );
	SpreadGeoInfo[2] = htons( numv );
	memcpy( &SpreadGeoInfo[3], modelmatrix, sizeof( float ) * 16 );
	SpreadPushMessage(geo->parent, 89, sizeof(SpreadGeoInfo), SpreadGeoInfo );
}


void SpreadFreeGeometry( SpreadGeometry * geo )
{
	if( geo->strides ) free( geo->strides );
	if( geo->types ) free( geo->types );
	if( geo->geoname ) free( geo->geoname );
	geo->geoname = 0;

	int i;
	for( i = 0; i < geo->numarrays; i++ )
	{
		free( geo->arrays[i] );
		SpreadHashRemove( geo->parent, "geodata#_#", geo->geo_in_parent, i );
	}

	SpreadMessage( geo->parent, "geometry#", "bb", geo->geo_in_parent, 90, geo->geo_in_parent );
	SpreadHashRemove( geo->parent, "geometry#", geo->geo_in_parent );
}


