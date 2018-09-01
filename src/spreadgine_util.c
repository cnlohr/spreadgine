//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

#include <string.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>
#include <stdlib.h>

#include <spatialloc.h>

static uint16_t * TVIndices;
static float * TVPositions;
static float * TVColors;
static float * TVTexCoords;
static float * TVNormals;
static int TVmax;
static int TVvertplace;
static int TVindexplace;
static int TVrender_type;

//This is an awful hideous function for glomming geometry on the end of this stuff.
int ImmediateModeMesh( struct SpreadGeometry * geo, float * trans44, float * coloroff, float * colorscale, float * tcoff, float * tcscale )
{
	if( TVindexplace + geo->indices >= TVmax )
	{
		fprintf( stderr, "Error: overrun of immediate mode mesh.\n" );
		return -2;
	}
	int verts = geo->verts;
	int i;
	void ** arrays = geo->arrays;
	uint8_t * strides = geo->strides;

	if( geo->numarrays > 0 && arrays[0] && strides[0] >= 3 ) for( i = 0; i < verts; i++ ) //Positions
	{
		int v = i + TVvertplace;
		float * posout = &TVPositions[v*3];
		float * posin = ((float*)arrays[0]) + i * strides[0];

		if( trans44 )
		{
			posout[0] = posin[0] * trans44[0] + posin[1] * trans44[4] + posin[2] * trans44[8] + trans44[12];
			posout[1] = posin[0] * trans44[1] + posin[1] * trans44[5] + posin[2] * trans44[9] + trans44[13];
			posout[2] = posin[0] * trans44[2] + posin[1] * trans44[6] + posin[2] * trans44[10] + trans44[14];
		}
		else
		{
			posout[0] = posin[0];
			posout[1] = posin[1];
			posout[2] = posin[2];
		}
	}
	else 
	{
		fprintf( stderr, "Error: arrays[0] doesn't exist. No geometry.\n" );
		return -1;
	}

	if( geo->numarrays > 1 && arrays[1] ) for( i = 0; i < verts; i++ ) //Colors
	{
		int v = i + TVvertplace;
		float * colout = &TVColors[v*4];
		float * colin = ((float*)arrays[1]) + i * strides[1];

		if( colorscale && coloroff )
		{
			colout[0] = (strides[1]>0)?(colin[0] * colorscale[0] + coloroff[0]):0;
			colout[1] = (strides[1]>1)?(colin[1] * colorscale[1] + coloroff[1]):0;
			colout[2] = (strides[1]>2)?(colin[2] * colorscale[2] + coloroff[2]):0;
			colout[3] = (strides[1]>3)?(colin[3] * colorscale[3] + coloroff[3]):0;
		}
		else
		{
			colout[0] = (strides[1]>0)?(colin[0]):0;
			colout[1] = (strides[1]>1)?(colin[1]):0;
			colout[2] = (strides[1]>2)?(colin[2]):0;
			colout[3] = (strides[1]>3)?(colin[3]):0;
		}
	}
	else for( i = 0; i < verts; i++ )
	{
		int v = i + TVvertplace;
		float * colout = &TVColors[v*4];
		colout[0] = 0;
		colout[1] = 0;
		colout[2] = 0;
		colout[3] = 0;
	}


	if( geo->numarrays > 2 && arrays[2] ) for( i = 0; i < verts; i++ ) //TexCoord
	{
		int v = i + TVvertplace;
		float * tcout = &TVTexCoords[v*4];
		float * tcin = ((float*)arrays[1]) + i * strides[1];

		if( tcscale && tcoff )
		{
			tcout[0] = (strides[2]>0)?(tcin[0] * tcscale[0] + tcoff[0]):1;
			tcout[1] = (strides[2]>1)?(tcin[1] * tcscale[1] + tcoff[1]):1;
			tcout[2] = (strides[2]>2)?(tcin[2] * tcscale[2] + tcoff[2]):1;
			tcout[3] = (strides[2]>3)?(tcin[3] * tcscale[3] + tcoff[3]):1;
		}
		else
		{
			tcout[0] = (strides[2]>0)?(tcin[0]):1;
			tcout[1] = (strides[2]>1)?(tcin[1]):1;
			tcout[2] = (strides[2]>2)?(tcin[2]):1;
			tcout[3] = (strides[2]>3)?(tcin[3]):1;
		}
	}
	else for( i = 0; i < verts; i++ )
	{
		int v = i + TVvertplace;
		float * tcout = &TVTexCoords[v*4];
		tcout[0] = 1;
		tcout[1] = 1;
		tcout[2] = 1;
		tcout[3] = 1;
	}

	if( geo->numarrays > 3 &&  arrays[3] && strides[3] >= 3 ) for( i = 0; i < verts; i++ ) //Normal
	{
		int v = i + TVvertplace;
		float * posout = &TVNormals[v*3];
		float * posin = ((float*)arrays[3]) + i * strides[3];

		if( trans44 )
		{
			posout[0] = posin[0] * trans44[0] + posin[1] * trans44[1] + posin[2] * trans44[2];
			posout[1] = posin[0] * trans44[4] + posin[1] * trans44[5] + posin[2] * trans44[6];
			posout[2] = posin[0] * trans44[8] + posin[1] * trans44[9] + posin[2] * trans44[10];
		}
		else
		{
			posout[0] = posin[0];
			posout[1] = posin[1];
			posout[2] = posin[2];
		}

	}
	else for( i = 0; i < verts; i++ )
	{
		int v = i + TVvertplace;
		float * tcout = &TVNormals[v*3];
		tcout[0] = 1;
		tcout[1] = 1;
		tcout[2] = 1;
	}


	int indices = geo->indices;
	for( i = 0; i < indices; i++ )
	{
		TVIndices[TVindexplace++] = geo->indexarray[i]+TVvertplace;
	}
	TVvertplace += verts;
	return 0;	
}

SpreadGeometry * CreateMeshGen( Spreadgine * spr, const char * geoname, int render_type, int max_iset )
{
	uint16_t * indexbuffer = malloc( sizeof( uint16_t ) * max_iset );
	void * arrays[4];
	int i;
	int TVstrideset[4] = { 3, 4, 4, 3 };
	int Types[4] = { GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT };
	arrays[0] = malloc( 3 * 4 * max_iset ); 
	arrays[1] = malloc( 4 * 4 * max_iset ); 
	arrays[2] = malloc( 4 * 4 * max_iset ); 
	arrays[3] = malloc( 3 * 4 * max_iset ); 
	const void * arrayout[4] = { arrays[0], arrays[1], arrays[2], arrays[3] };
	SpreadGeometry * ret = SpreadCreateGeometry( spr, geoname, render_type, max_iset, indexbuffer, max_iset, 4, arrayout, TVstrideset, Types );
	if( ret ) ret->max_set = max_iset;
	return ret;
}

void StartImmediateMode( SpreadGeometry * geo )
{
	TVmax = geo->max_set;
	TVvertplace = 0;
	TVindexplace = 0;
	TVIndices = geo->indexarray;
	TVPositions = (float*)geo->arrays[0];
	TVColors = (float*)geo->arrays[1];
	TVTexCoords = (float*)geo->arrays[2];
	TVNormals = (float*)geo->arrays[3];	
}

void UpdateMeshToGen( SpreadGeometry * geo )
{
	geo->laststartv = 0;
	geo->indices = TVindexplace;
	geo->verts   = TVvertplace;

	UpdateSpreadGeometry( geo, -2, 0 );

	TVvertplace = 0;
	TVindexplace = 0;
	TVmax = 0;
}

SpreadGeometry * MakeSquareMesh( Spreadgine * e, int w, int h )
{
	int i;
	int x, y;
	int c = w * h;
	int v = (w+1)*(h+1);
	uint16_t indices[6*c];
	float points[v*3];
	float colors[v*4];
	float texcoord[v*4];
	float normals[v*3];

	for( y = 0; y < h; y++ )
	for( x = 0; x < w; x++ )
	{
		int i = x + y * w;
		indices[i*6+0] = x + y * (w+1);
		indices[i*6+1] = (x+1) + y * (w+1);
		indices[i*6+2] = (x+1) + (y+1) * (w+1);
		indices[i*6+3] = (x) + (y) * (w+1);
		indices[i*6+4] = (x+1) + (y+1) * (w+1);
		indices[i*6+5] = (x) + (y+1) * (w+1);
	}
	for( y = 0; y <= h; y++ )
	for( x = 0; x <= w; x++ )
	{
		int p = x+y*(w+1);
		colors[p*4+0] = texcoord[p*4+0] = points[p*3+0] = x/(float)w;
		colors[p*4+1] = texcoord[p*4+1] = points[p*3+1] = y/(float)h;
		colors[p*4+2] = texcoord[p*4+2] = points[p*3+2] = 0;
		colors[p*4+3] = texcoord[p*4+3] = 1;

		normals[p*3+0] = 0;
		normals[p*3+1] = 0;
		normals[p*3+2] = -1;
	}

	const void * arrays[4] = { (void*)points, (void*)colors, (void*)texcoord, (void*)normals };
	int strides[4] = { 3, 4, 4, 3 };
	int types[4] = { GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT };

	return SpreadCreateGeometry( e, "plat2", GL_TRIANGLES, 6*c, indices, v, 4, arrays, strides, types );
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

BatchedSet * CreateBatchedSet( Spreadgine * spr, const char * setname, int max_objects, int max_indices, int render_type , int texturex, int texturey )
{
	char ct[1024];
	BatchedSet * ret = malloc( sizeof( BatchedSet ) );
	ret->allocated_indices = calloc( max_indices, 1 );
	ret->allocated_vertices = calloc( max_indices, 1 );
	ret->spr = spr;
	ret->setname = strdup( setname );
	snprintf( ct, sizeof(ct)-1, "%s_tex", setname );
	ret->associated_texture = SpreadCreateTexture( spr, ct, texturex, texturey, 4, GL_UNSIGNED_BYTE );
	ret->spatial_allocator = SpatCreate( texturex, texturey );
	snprintf( ct, sizeof(ct)-1, "%s_geo", setname );
	ret->coregeo = CreateMeshGen( spr, ct, render_type, max_indices );
	ret->objects = calloc( max_objects, sizeof( BatchedObject ) );
	ret->max_objects = max_objects;
	ret->max_index = max_indices;
	return ret;
}

void FreeBatchedSet( BatchedSet * set )
{
	int i;
	free( set->setname );
	free( set->allocated_indices );
	free( set->allocated_vertices );
	for( i = 0; i < max_objects; i++ )
	{
		FreeBatchedObject( &set->objects[i] );
	}
	SpreadFreeTexture( set->associated_texture );
	SpatDestroy( set->spatial_allocator );
	SpreadFreeGeometry( set->coregeo );
	free( set );
}

void FreeBatchedObject( BatchedObject * o )
{
	if( !o ) return;
	if( !o->name ) free( o->name );
	parent->objects[o->objinparent] = 0;

	int vstart = o->which_vertex_place;
	int istart = o->which_index_place;
	while( vstart > 0 && parent->allocated_vertices[vstart] != 1 ) vstart--;
	while( istart > 0 && parent->allocated_indices[istart] != 1 ) istart--;
	memset( parent->allocated_vertices + vstart, 0, o->vertices );
	memset( parent->allocated_indices + istart, 0, o->indices );
	SpatFree( parent->spatial_allocator, o->texturestartx, o->texturestarty );
	struct ExtraTexs * et = o->extratex;
	while( et )
	{
		SpatFree( parent->spatial_allocator, et->x, et->y );
		struct ExtraTexs * del = et;
		et = et->next;
		free( del );
	}
	free( o );
}

void UpdateBatchedObjectTransformData( BatchedObject * o, const float * Position, const float * Quaternion, float scale )
{
	int x = o->texturestartx;
	int y = o->texturestarty;

	SpreadTexture * tex = o->parent->associated_texture;
	uint8_t tpd[16];
	uint16_t pixset[8] = {
		Position[0] * 1000 + 32767,
		Position[1] * 1000 + 32767,
		Position[2] * 1000 + 32767,
		scale       * 1000 + 32767, 
		Quaternion[0] * 32767 + 32767,
		Quaternion[1] * 32767 + 32767,
		Quaternion[2] * 32767 + 32767,
		Quaternion[3] * 32767 + 32767 };

	int i;
	for( i = 0; i < 8; i++ )
	{
		tpd[i * 8 + 0] = pixset[i] >> 8;
		tpd[i * 8 + 8] = pixset[i] & 0xff;
	}
	SpreadUpdateSubTexture( tex, tpd, x, y, 4, 1 );
}


BatchedObject * AllocateBatchedObject( BatchedSet * set, SpreadGeometry * object, const char * name )
{
	//Part1: Need to find a window of vertices + indices big enough.
	int indexstart;
	int vertexstart;
	int needed_index = object->indices;
	int needed_vert = object->verts;
	int i;
	{
		int streak = 0;
		int max_index = set->max_index;

		for( i = 0; i < max_index; i++ )
		{
			if( set->allocated_indices[0] == 0 ) streak++; else streak = 0;
			if( streak == needed_index ) break;
			if( streak == 1 ) indexstart = i;
		}
		if( i == max_index )
		{
			fprintf( stderr, "Could not allocated index for %s\n", name );
			return 0;
		}

		for( i = 0; i < max_index; i++ )
		{
			if( set->allocated_vertices[0] == 0 ) streak++; else streak = 0;
			if( streak == needed_vertex ) break;
			if( streak == 1 ) vertexstart = i;
		}
		if( i == max_index )
		{
			fprintf( stderr, "Could not allocated vertex for %s\n", name );
			return 0;
		}

	}

	//Ok, we have indexstart and vertexstart.
	//Next, allocate the texture space.
	int texx, texy;
	if( SpatMalloc( set->spatial_allocator, 4, 1, &texx, &texy ) )
	{
		fprintf( stderr, "Could not allocated texture for %s\n", name );
		return 0;
	}

	int objin = 0;
	for( i = 0; i < set->max_objects; i++ )
	{
		if( set->objects[i] == 0 ) break;
	}

	if( i == set->max_objects )
	{
		fprintf( stderr, "Could not allocated object for %s\n", name );
		return 0;
	}

	BatchedObject * ret = malloc( sizeof( BatchedObject ) );
	ret->which_index_place = indexstart;
	ret->which_vertex_place = vertexstart; 
	ret->texturestartx = texx;
	ret->texturestarty = texy;
	ret->objinparent = objin;
	ret->indices = needed_index;
	ret->vertices = needed_vert;
	ret->name = strdup(name);
	ret->parent = set;
	ret->extratex = 0;
}

