#ifndef _SPREADGINE_H
#define _SPREADGINE_H

#include <stdint.h>

#ifndef FLT
#define FLT float
#endif

typedef struct SpreadShader;
typedef struct SpreadGeometry;
typedef struct SpreadTexture;

typedef struct
{
	SpreadShader * shaders;
	int maxshaders;
	SpreadGeometry * geometries;
	int maxgeos;
	SpreadTexture * textures;
	int maxtexs;

	//Matrix management appens in CNFG3D.h
} Spreadgine;

Spredgine * SpreadInit( int w, int h, const char * title, int httpport );
void SpreadDestroy( Spreadgine * spr );s


//
void spglEnable( Spredgine * e, int en );
void spglDisable( Spredgine * e,int en );
void spglLineWidth( Spredgine * e,float wid );
void spglSwap(Spredgine * e);
void spglClearColor( Spredgine * e, float r, float g, float b, float a );
void spglClear( Spredgine * e, int clearmask );

//TODO: Add more GL stand-ins here.  Note that only functions available in GLES, GL and WebGL should be made available here.


/////////////////////////////SHADERS///////////////////////////////

typedef struct
{
	Spreadgine * parent;
	int shader_in_parent;
	char * shader_name;
	char ** uniform_slot_names;
	int known_uniform_slots;
	FLT ** uniforms_in_slots;
	int * size_of_uniforms;
	int modelview_index;
	int perspective_index;
} SpreadShader;

SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shader_name, const char * fragmentShader, const char * vertexShader );
int SpreadGetSlot( SpreadShader * shd, const char * slotname );
void SpreadUniform4f( SpreadShader * shd, int slot, float * uni );
void SpreadUniform16f( SpreadShader * shd, int slot, float * uni );
void SpreadFreeShader( SpreadShader * shd );
void SpreadApplyShader( SpreadShader * shd );
void SpreadDestroyShader( SpreadShader * shd );

//////////////////////////////GEOMETRY/////////////////////////////
typedef struct
{
	Spreadgine * parent;
	int geo_in_parent;

	float ** arrays;
	int * strides;
	int * types; //always GL_FLOAT except in extreme cases.
	int render_type; //GL_TRIANGLES, GL_POINTS, GL_LINES
	int numarrays;
	int arraylength;
} SpreadGeometry;

SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, 
void SpreadRenderGeometry( SpreadGeometry * geo, int start, int nr_emit ); 

//////////////////////////////TEXTURES//////////////////////////////

typedef struct
{
	Spreadgine * parent;
	int texture_in_parent;

	int channels;
	int mode;
	uint8_t * pixeldata;
} SpreadTexture;

SpreadTexture * SpreadCreateTexture( Spreadgine * spr, int x, int y, int chan, int mode );
SpreadTexture * SpreadLoadTexture( Spreadgine * spr, const char * tfile );
void SpreadApplyTexture( SpreadTexture * tex, int slot );

#endif

//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.
