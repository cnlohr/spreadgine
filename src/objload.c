//Copyright 2013, 2018 <>< Charles Lohr, Relicensed under the MIT/x11 License.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <spreadgine.h>
#include <ctype.h>


#define TBUFFERSIZE 256
#define VBUFFERSIZE 524288
#define MAX_OBJECTS_PER_OBJ 128

//This file is ripped off of 'bridgesim' so it should probably be re-written to efficiently read OBJs into the spreadgine.


struct OBJObject
{
	const char * OName;

	int VertCount;  //3 floats per vert count
	float * Vertices;

	int NormalCount; //3 floats per normal count
	float * Normals;
	//float * Tangents; //4 floats per.

	int TexCount;    //3 floats per tex
	float * Textures;

	int TriCount;   //3 ints per face index, i.e. 9 per face... [vertex texture norm] [vertex texture norm]
	int * Faces;

	float mins[3];
	float maxs[3];
	float center[3];

	unsigned int OGLList; //Optionally implemented by user, may have OpenGL List.

	struct GPUGeometry * geometry;
};

struct OBJFile
{
	struct TempObject * t;
	const char * Filename;
	int NumObjects;
	struct OBJObject * Objects[MAX_OBJECTS_PER_OBJ];
};


struct TempObject
{
	char  CNameBuffer[TBUFFERSIZE];
	int   CVertCount;
	float CVerts[VBUFFERSIZE*3];
	int   CNormalCount;
	float CNormals[VBUFFERSIZE*3];
	int   CTexCount;
	float CTexs[VBUFFERSIZE*3];
	int   CTriCount;
	int   CFaces[VBUFFERSIZE*9];
};

static void ResetTempObject( struct TempObject * o )
{
	memset( o, 0, sizeof( struct TempObject ) );
}

#if 0
static void CommitObject( struct TempObject * o, struct OBJFile * f )
{
	int i, j;
	struct OBJObject * oo = malloc( sizeof( struct OBJObject ) );
	f->Objects[f->NumObjects++] = oo;

	oo->OName = strdup( o->CNameBuffer );
	oo->VertCount = o->CVertCount;
	oo->Vertices = malloc( oo->VertCount * sizeof( float ) * 3 );
	memcpy( oo->Vertices, o->CVerts, oo->VertCount * sizeof( float ) * 3 );
	oo->NormalCount = o->CNormalCount;
	oo->Normals = malloc( oo->NormalCount * sizeof( float ) * 3 );
	memcpy( oo->Normals, o->CNormals, oo->NormalCount * sizeof( float ) * 3 );
	oo->TexCount = o->CTexCount;
	oo->Textures = malloc(o->CTexCount * sizeof( float ) * 3 );
	memcpy( oo->Textures, o->CTexs, o->CTexCount * sizeof( float ) * 3 );
	oo->TriCount = o->CTriCount;
	oo->Faces = malloc( o->CTriCount * sizeof( int ) * 9 );
	memcpy( oo->Faces, o->CFaces, o->CTriCount * sizeof( int ) * 9 );

	for( j = 0; j < 3; j++ )
	{
		oo->mins[j] = 1e20;
		oo->maxs[j] = -1e20;
	}

	for( i = 0; i < oo->VertCount; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			float t = oo->Vertices[j + i*3];
			if( t < oo->mins[j] ) oo->mins[j] = t;
			if( t > oo->maxs[j] ) oo->maxs[j] = t;
		}
	}

	for( j = 0; j < 3; j++ )
	{
		oo->center[j] = ( oo->maxs[j] + oo->mins[j] ) / 2.f;
	}

	//Make OPENGLized geometry.
	//Technique: If vertex, texture and normal match, this is a valid index.
	//  If something doesn't match up, need to make a new entry.
	float * OutputVerts = 0; 
	float * OutputTexs = 0;
	float * OutputNorms = 0;
	int *   OutputIndices = malloc( oo->TriCount * 3 * sizeof( float ) );

	if( oo->VertCount )  OutputVerts = malloc( oo->TriCount * 3 * sizeof( float ) * 3 );
	if( oo->TexCount )   OutputTexs  = malloc( oo->TriCount * 3 * sizeof( float ) * 3 );
	if( oo->NormalCount) OutputNorms = malloc( oo->TriCount * 3 * sizeof( float ) * 3 );
	int OutputVertCount = 0;

	for( i = 0; i < oo->TriCount*3; i++ )
	{
		int match = -1;

		int indexV = oo->Faces[i*3+0];
		int indexT = oo->Faces[i*3+1];
		int indexN = oo->Faces[i*3+2];

		float * pos = &oo->Vertices[indexV*3];
		float * tex = &oo->Textures[indexT*3];
		float * nrm = &oo->Normals[indexN*3];

		if( oo->VertCount == 0 )   pos = 0;
		if( oo->TexCount == 0 )    tex = 0;
		if( oo->NormalCount == 0 ) nrm = 0;

	//	for( j = 0; j < OutputVertCount; j++ )
	//	{
	//		int m1 = compare3d( pos, &OutputVerts[j*3], DEFAULT_EPSILON );
	//		m1 |= compare3d( tex, &OutputTexs[j*3], DEFAULT_EPSILON );
	//		m1 |= compare3d( nrm, &OutputNorms[j*3], DEFAULT_EPSILON );
	//		if( m1 ) continue;
	//	}

		//if( j == OutputVertCount )
		{
			memcpy( &OutputVerts[j*3], pos, sizeof( float ) * 3 );
			if( tex )
				memcpy( &OutputTexs[j*3], tex, sizeof( float ) * 3 );
			if( nrm )
				memcpy( &OutputNorms[j*3], nrm, sizeof( float ) * 3 );
			OutputVertCount++;
		}

		OutputIndices[i] = j;
	} 

//	float * OutputTangents = 0;
//	if( OutputVerts && OutputNorms && OutputTexs )
//		OutputTangents = CalculateTangentSpace( o->CTriCount, OutputVertCount, OutputIndices, OutputVerts, OutputNorms, OutputTexs );

	int vertentrycount = 1;
	struct VertexData ** vertslist = malloc( sizeof( struct VertexData * ) * 4 );
	char * names[4];
	
//	struct IndexData * indices = IndexDataCreate();
//	UpdateIndexData( indices, o->CTriCount * 3, OutputIndices );

//	struct VertexData * positions = VertexDataCreate();
//	UpdateVertexData( positions, OutputVerts, OutputVertCount, 3 );
//	names[0] = 0;
//	vertslist[0] = positions;

	if( OutputTexs )
	{
	//	struct VertexData * texs = VertexDataCreate();
	//	UpdateVertexData( texs, OutputTexs, OutputVertCount, 3 );
		names[vertentrycount] = strdup( "texture" );
	//	vertslist[vertentrycount] = texs;
	//	vertentrycount++;
	}

	if( OutputNorms )
	{
	//	struct VertexData * norms = VertexDataCreate();
	//	UpdateVertexData( norms, OutputNorms, OutputVertCount, 3 );

		names[vertentrycount] = strdup( "normal" );
	//	vertslist[vertentrycount] = norms;
	//	vertentrycount++;
	}
/*	if( OutputTangents )
	{
		struct VertexData * tans = VertexDataCreate();
		UpdateVertexData( tans, OutputTangents, OutputVertCount, 4 );

		names[vertentrycount] = strdup( "tangent" );
		vertslist[vertentrycount] = tans;
		vertentrycount++;		
	}*/

	free( OutputIndices );
	if( OutputVerts ) free( OutputVerts );
	if( OutputTexs ) free( OutputTexs );
	if( OutputNorms ) free( OutputNorms );
//	if( OutputTangents ) free( OutputTangents );


	//oo->geometry = CreateGeometry( indices, vertentrycount, vertslist, names, 1, GL_TRIANGLES );
	//copy3d( oo->geometry->maxs, oo->maxs );
	//copy3d( oo->geometry->mins, oo->mins );

	for( i = 1; i < vertentrycount; i++ )
	{
		free( names[i] );
	}
}
#endif

static  int ReadOneLine( FILE * f, char * buffer )
{
	int c;
	int i = 0;
	while( ( c = fgetc( f ) ) != EOF )
	{
		if( c == '\r' ) continue;
		if( c == '\n' ) break;

		buffer[i++] = c;
	}
	buffer[i] = 0;
	return i;
}

static struct OBJFile * OpenOBJInternal( const char * Filename, int flipv )
{
	char  LineBuffer[TBUFFERSIZE];
	struct TempObject * t = malloc( sizeof( struct TempObject ) );;
	int CurrentVertOffset = 0;
	int CurrentNormOffset = 0;
	int CurrentTexOffset = 0;
	ResetTempObject( t );

	FILE * f = fopen( Filename, "r" );
	if( !f )
	{
		fprintf( stderr, "Error: cannot open file.\n" );
		return 0;
	}

	struct OBJFile * ret = malloc( sizeof( struct OBJFile ) );
	ret->Filename = strdup( Filename );
	ret->NumObjects = 0;


	while( !feof( f ) && !ferror( f ) )
	{
		int r = ReadOneLine( f, LineBuffer );

		if( r < 2 ) continue;
		if( tolower( LineBuffer[0] ) == 'v' )
		{
			if( tolower( LineBuffer[1] ) == 'n' )
			{
				int r = sscanf( LineBuffer + 3, "%f %f %f", 
					&t->CNormals[0 + t->CNormalCount * 3], 
					&t->CNormals[1 + t->CNormalCount * 3], 
					&t->CNormals[2 + t->CNormalCount * 3] );

				if( r == 3 )
					t->CNormalCount++;
			}
			else if( tolower( LineBuffer[1] ) == 't' )
			{
				int r = sscanf( LineBuffer + 3, "%f %f %f", 
					&t->CTexs[0 + t->CTexCount * 3], 
					&t->CTexs[1 + t->CTexCount * 3], 
					&t->CTexs[2 + t->CTexCount * 3] );

				if( flipv )
					t->CTexs[1 + t->CTexCount * 3] = 1. - t->CTexs[1 + t->CTexCount * 3];
				if( r == 3 || r == 2 )
					t->CTexCount++;
				else
				{
					fprintf( stderr, "Error: Invalid Tex (%d) (%s)\n", r, LineBuffer + 3 );
				}

			}
			else
			{
				int r = sscanf( LineBuffer + 2, "%f %f %f", 
					&t->CVerts[0 + t->CVertCount * 3], 
					&t->CVerts[1 + t->CVertCount * 3], 
					&t->CVerts[2 + t->CVertCount * 3] );

				if( r == 3 )
					t->CVertCount++;
				else
				{
					fprintf( stderr, "Error: Invalid Vertex\n" );
				}
			}
		}
		else if( tolower( LineBuffer[0] ) == 'f' )
		{
			char buffer[3][TBUFFERSIZE];
			int p = 0;
			int facemark = t->CTriCount * 9;
			int r = sscanf( LineBuffer + 1, "%30s %30s %30s", 
				buffer[0], buffer[1], buffer[2] );

			if( r == 0 )
			{
				//Invalid line - continue.
				continue;
			}
			else if( r == 1 )
			{
				memcpy( buffer[1], buffer[0], TBUFFERSIZE );
				memcpy( buffer[2], buffer[0], TBUFFERSIZE ); 
			}
			else if( r == 2 )
			{
				memcpy( buffer[2], buffer[1], TBUFFERSIZE ); 
			}

			//Whatever... they're all populated with something now, even if it is largely useless.

			for( p = 0; p < 3; p++ )
			{
				char buffer2[3][TBUFFERSIZE];
				int mark = 0, markb = 0;
				int i;
				int sl = strlen( buffer[p] );
				for( i = 0; i < sl; i++ )
				{
					if( buffer[p][i] == '/' )
					{
						buffer2[mark][markb] = 0;
						mark++;
						if( mark >= 3 ) break;
						markb = 0;
					}
					else
						buffer2[mark][markb++] = buffer[p][i];
				}
				buffer2[mark][markb] = 0;
				for( i = mark+1; i < 3; i++ )
					buffer2[i][0] = 0;

				t->CFaces[facemark++] = atoi( buffer2[0] ) - 1 - CurrentVertOffset;
				t->CFaces[facemark++] = atoi( buffer2[1] ) - 1 - CurrentTexOffset;
				t->CFaces[facemark++] = atoi( buffer2[2] ) - 1 - CurrentNormOffset;
			}

			t->CTriCount++;
		}
		else if( tolower( LineBuffer[0] ) == 'o' )
		{
			//XXX TODO: If you want to support multiple objects, uncomment this
#if 0
			if( t->CFaces && t->CTriCount)
			{
				CurrentVertOffset += t->CVertCount;
				CurrentNormOffset += t->CNormalCount;
				CurrentTexOffset += t->CTexCount;
				CommitObject( t, ret );
				ResetTempObject( t );
			}

			memcpy( t->CNameBuffer, &LineBuffer[2], strlen( &LineBuffer[2] ) );
#endif
		}
		else if( strncmp( LineBuffer, "usemtl", 6 ) == 0 )
		{
			//Not implemented.
		}
		else if( strncmp( LineBuffer, "mtllib", 6 ) == 0 )
		{
			//Not implemented.
		}
		else if( tolower( LineBuffer[0] ) == 's' )
		{
			//Not implemented.
		}
	}

/*	if( t->CFaces )
	{
		CommitObject( t, ret );
	}*/

	fclose( f );
	//free( t );
	ret->t = t;

	return ret;
}


//struct OBJFile * OpenOBJInternal( const char * Filename, int flipv );


SpreadGeometry * LoadOBJ( Spreadgine * spr, const char * filename, int flipv, int make_wireframe )
{
	struct OBJFile * ofi = OpenOBJInternal( filename, flipv );
	if( !ofi )
	{
		fprintf( spr->fReport, "Failed to open \"%s\".\n" );
		return 0;
	}
	struct TempObject * of = ofi->t;
	if( !of )
	{
		fprintf( spr->fReport, "Failed to get object from \"%s\".\n" );
		return 0;
	}

	int icount = of->CTriCount * 3 * (make_wireframe?2:1);
	int ivcount = of->CTriCount * 3;
	//This is a very inefficent way of doing it, since we are effectively re-copying vertices for
	//each triangle :-/.  But, I can't think of a better way at the minute to do this and also
	//allow for keeping distinct vertices and edges.

	if( icount > 65535 )
	{
		icount = 65535;
	}
	const char * name = of->CNameBuffer[0]?of->CNameBuffer:filename;
	uint16_t * Indices = malloc( icount * sizeof( uint16_t ) );
	float * Vertices = of->CVerts?malloc(  ivcount * sizeof( float ) * 3 ):0;
	float * Normals = of->CNormals?malloc( ivcount * sizeof( float ) * 3 ):0;
	float * Textures = of->CTexs?malloc(   ivcount * sizeof( float ) * 3 ):0;
	int i;
	for( i = 0; i < ivcount; i++ )
	{
		int iev = of->CFaces[i*3+0];
		int ien = of->CFaces[i*3+2];
		int iet = of->CFaces[i*3+1];
		if( make_wireframe )
		{
			Indices[i*2+0] = i;
			Indices[i*2+1] = ((i%3)==2)?(i-2):(i+1);
		}
		else
		{
			Indices[i] = i;
		}
		printf( "%d %d %d\n", iev, ien, iet );
		if( Vertices ) memcpy( &Vertices[i*3], &of->CVerts[iev*3], sizeof(float)*3 );
		if( Normals ) memcpy( &Normals[i*3], &of->CNormals[ien*3], sizeof(float)*3 );
		if( Textures ) memcpy( &Textures[i*3], &of->CTexs[iet*3], sizeof(float)*3 );
	}

	//Free OBJ file
	for( i = 0; i < ofi->NumObjects; i++ )
	{
		struct OBJObject * o = ofi->Objects[i];
		if( o->Vertices ) free( o->Vertices );
		if( o->Normals ) free( o->Normals );
		if( o->Textures ) free( o->Textures );
		if( o->Faces ) free( o->Faces );
		free( o );
	}
	
	const void * arrays[3];
	int strides[3] = { 3, 3, 3 };
	int types[3] = { GL_FLOAT, GL_FLOAT, GL_FLOAT };
	int nrarray = 0;
	if( Vertices ) { arrays[nrarray++] = Vertices; }
	if( Normals )  { arrays[nrarray++] = Normals; }
	if( Textures ) { arrays[nrarray++] = Textures; }
printf( "CCGGGG %d\n", nrarray );

	SpreadGeometry * ret = SpreadCreateGeometry( spr, filename, make_wireframe?GL_LINES:GL_TRIANGLES, icount, Indices, ivcount, nrarray, arrays, strides, types );

	free( of );
/*
	if( Vertices ) free( Vertices );
	if( Indices ) free( Indices );
	if( Normals ) free( Normals );
	if( Textures ) free( Textures );
*/
	return ret;
}



