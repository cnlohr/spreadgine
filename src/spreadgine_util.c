//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

#include <string.h>
#include <spreadgine.h>
#include <spreadgine_remote.h>
#include <stdlib.h>

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

		colors[p*4+2] = 1;
	}

	const void * arrays[3] = { (void*)points, (void*)colors, (void*)texcoord };
	int strides[3] = { 3, 4, 4 };
	int types[3] = { GL_FLOAT, GL_FLOAT, GL_FLOAT };

	return SpreadCreateGeometry( e, "plat2", GL_TRIANGLES, 6*c, indices, v, 3, arrays, strides, types );
}

