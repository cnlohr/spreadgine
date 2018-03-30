#ifndef _SPREADGINE_H
#define _SPREADGINE_H

#include <stdint.h>

typedef struct SpreadShader;
typedef struct SpreadGeometry;
typedef struct SpreadTexture;


#define SPREADGINE_VIEWPORTS 2
#define SPREADGINE_CAMERAS   10
#define SPREADGINE_CIRCBUF   65536*16	//A 1MB circular buffer.


typedef struct
{
	SpreadShader * shaders;
	int setshaders;
	SpreadGeometry * geometries;
	int setgeos;
	SpreadTexture * textures;
	int settexs;

	//Matrix management appens in CNFG3D.h
	float (*vpperspectives)[16];  //Perspective matrix per-eye.
	float (*vpviews)[16];   //View matrix per-eye.
	char ** vpnames;
	int   vpedges[SPREADGINE_VIEWPORTS][4]; //x,y,w,h
	int   setvps;

	FILE * fReport;

	//Internal.  NO TOUCHIE!
	int		cbhead;	//Head counts up all the way to int_max.  Be sure to modulus whenever indexing into spreadthread.
	uint8_t * cbbuff;
	void * spreadthread;
	float  lastclearcolor[4];
} Spreadgine;

//Initializes the spreadgine with a basic shader 'default', a cube 'cube', and a texture 'gradiant'
//It also initializes `vps` views with camera at the origin.
//Be sure to set everything up before a client connects. None of these functions will update the client.
Spredgine * SpreadInit( int w, int h, const char * title, int httpport, int vps, FILE * fReport );
void SpreadDestroy( Spreadgine * spr );
void SpreadSetupCamera( Spreadgine * spr, int camid, const char * camname );


void spglEnable( Spredgine * e, int en );
void spglDisable( Spredgine * e, int en );
void spglLineWidth( Spredgine * e, float wid );
void spglSwap(Spredgine * e);
void spglClearColor( Spredgine * e, float r, float g, float b, float a );
void spglClear( Spredgine * e, int clearmask );
//TODO: Add more GL stand-ins here.  Note that only functions available in GLES, GL and WebGL should be made available here.



/////////////////////////////CORE TO-BROWSER///////////////////////
//
//Messages:
//	64 range
//		64 = InitializationPacket( uint32_t 
//		65 = UpdateCameraName( uint8_t id, char name[...] );
//		66 = UpdateCameraPerspective( uint8_t id, float perspective[16] );
//		67 = UpdateCameraView( uint8_t id, float view[16] );
//
//		74 = glEnable( int )
//		75 = glDisable( int )
//		76 = glLineWidth( float )
//		77 = glSwap
//		78 = glClearColor( float, float, float, float )
//		79 = glClear( int )
//
//  128+ = User functions
void SpreadPushMessage( Spredgine * e, uint8_t messageid, int payloadsize, void * payload );

/////////////////////////////SHADERS///////////////////////////////

typedef struct
{
	Spreadgine * parent;
	int shader_in_parent;
	char * shader_name;

	char ** uniform_slot_names;
	int known_uniform_slots;
	float ** uniforms_in_slots;
	int * size_of_uniforms;

	//"Required" uniform matrix entries.
	int model_index;
	int modelview_index;
	int perspective_index;

	int vertex_shader;
	int fragment_shader;
	int program_shader;
} SpreadShader;

SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shader_name, const char * fragmentShader, const char * vertexShader, int attriblistlength, const char ** attriblist );
int SpreadGetUniformSlot( SpreadShader * shd, const char * slotname );
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
	char * geoname;

	float ** arrays;
	int * strides;
	int * types; //always GL_FLOAT except in extreme cases.
	int render_type; //GL_TRIANGLES, GL_POINTS, GL_LINES
	int numarrays;	//First array is always 
	int arraylength;
} SpreadGeometry;

SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, 
void SpreadRenderGeometry( SpreadGeometry * geo, int start, int nr_emit ); 

//////////////////////////////TEXTURES//////////////////////////////

typedef struct
{
	Spreadgine * parent;
	int texture_in_parent;
	char * texname;

	int channels;
	int mode;
	uint8_t * pixeldata;
} SpreadTexture;

SpreadTexture * SpreadCreateTexture( Spreadgine * spr, int x, int y, int chan, int mode );
SpreadTexture * SpreadLoadTexture( Spreadgine * spr, const char * tfile );
void SpreadApplyTexture( SpreadTexture * tex, int slot );

#endif

//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.
