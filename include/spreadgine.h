//Copyright <>< 2018 Charles Lohr, under the MIT-x11 or NewBSD licenses, you choose.

#ifndef _SPREADGINE_H
#define _SPREADGINE_H

#include <stdint.h>
#include <stdio.h>
#ifdef EGL
#include <EGL/egl.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif

typedef struct SpreadShader SpreadShader;
typedef struct SpreadGeometry SpreadGeometry;
typedef struct SpreadTexture SpreadTexture;
typedef struct Spreadgine Spreadgine;


#define SPREADGINE_VIEWPORTS 2
#define SPREADGINE_CAMERAS   10
#define SPREADGINE_CIRCBUF   65536*16	//A 1MB circular buffer.


struct Spreadgine
{
	SpreadShader * shaders;
	int setshaders;
	SpreadGeometry * geos;
	int setgeos;
	SpreadTexture * textures;
	int settexs;

	int current_shader;

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
};


//Initializes the spreadgine with a basic shader 'default', a cube 'cube', and a texture 'gradiant'
//It also initializes `vps` views with camera at the origin. For 99.9% of applications, vps should be 2.
//Be sure to set everything up before a client connects. None of these functions will update the client.
//Also... 'fReport' can either be stderr or something you make with open_memstream
Spreadgine * SpreadInit( int w, int h, const char * title, int httpport, int vps, FILE * fReport );
void SpreadDestroy( Spreadgine * spr );
void SpreadSetupCamera( Spreadgine * spr, int camid, const char * camname );


void spglEnable( Spreadgine * e, uint32_t en );
void spglDisable( Spreadgine * e, uint32_t de );
void spglLineWidth( Spreadgine * e, float wid );
void spglSwap(Spreadgine * e);
void spglClearColor( Spreadgine * e, float r, float g, float b, float a );
void spglClear( Spreadgine * e, uint32_t clearmask );
//TODO: Add more GL stand-ins here.  Note that only functions available in GLES, GL and WebGL should be made available here.



/////////////////////////////CORE TO-BROWSER///////////////////////
//
//Messages: [message size (Does not include opcode)] [opcode] [payload (optional)]
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
//		80 = glUseProgram( Shader[int]->shader_in_parent );
//		81 = glUniform4fv( float[4] + plan_text_uniform_name );
//		82 = glUniformMatrix4fv( float[4] + plan_text_uniform_name );
//		83..87 reserved for other uniform operations.
//		88 = PushNewArrayData( int geono, int arrayno, [VOID*] data);
//		89 = SpreadRenderGeometry( int geono, int offset_at, int nr_verts, float viewmatrix[16] );

//  128+ = User functions
void SpreadPushMessage( Spreadgine * e, uint8_t messageid, int payloadsize, void * payload );


//Bump configuration means something about the overarching configuration has changed.
//Tell clients to re-download the new, updated description.
void SpreadBumpConfiguration( Spreadgine * e );

/////////////////////////////SHADERS///////////////////////////////

struct SpreadShader
{
	Spreadgine * parent;
	uint32_t shader_in_parent;
	char * shadername;

	char * fragment_shader_text;
	char * vertex_shader_text;

	//Need a way to translate uniform locations to client monitors...
	//These use "index" as the slot.
	char ** uniform_slot_names;
	int  * uniform_slot_name_lens;
	int known_uniform_slots;
	float ** uniforms_in_slots;
	int * size_of_uniforms;

	//"Required" uniform matrix entries.
	int model_index;  // YOU MUST INCLUDE "mmatrix" so spreadgine knows where to shove the camera.
	int view_index;  // YOU MUST INCLUDE "vmatrix" so spreadgine knows where to shove the camera.
	int perspective_index; // YOU MUST INCLUDE "pmatrix" so spreadgine knows where to shove the camera.

	//Internal backend types.
	int vertex_shader;
	int fragment_shader;
	int program_shader;
};

//Attriblist == the positions of the geometry attributes in your geometry, usually [vpos] [vcolor] or something like that.
SpreadShader * SpreadLoadShader( Spreadgine * spr, const char * shadername, const char * fragmentShader, const char * vertexShader, int attriblistlength, const char ** attriblist );
int SpreadGetUniformSlot( SpreadShader * shd, const char * slotname );
void SpreadUniform4f( SpreadShader * shd, int slot, const float * uni );
void SpreadUniform16f( SpreadShader * shd, int slot, const float * uni );
void SpreadFreeShader( SpreadShader * shd );
void SpreadApplyShader( SpreadShader * shd );

//////////////////////////////GEOMETRY/////////////////////////////
struct SpreadGeometry
{
	Spreadgine * parent;
	uint32_t geo_in_parent;
	char * geoname;

	void ** arrays;
	int * strides;
	int * typesizes; //Almost always array of [4]'s
	int * types; //always GL_FLOAT except in extreme cases.
	int render_type; //GL_TRIANGLES, GL_POINTS, GL_LINES
	int numarrays;	//First array is always; specifies the number of types of arays.
	int verts;
};

SpreadGeometry * SpreadCreateGeometry( Spreadgine * spr, const char * geoname, int render_type, int verts, int nr_arrays, const void ** arrays, int * strides, int * types, int * typesizes );
void UpdateSpreadGeometry( SpreadGeometry * geo, int arrayno, void * arraydata );
void SpreadRenderGeometry( SpreadGeometry * geo, int start, int nr_emit, const float * modelmatrix ); 

//////////////////////////////TEXTURES//////////////////////////////

//Not implementing textures yet.
/*
typedef struct
{
	Spreadgine * parent;
	uint32_t texture_in_parent;
	char * texname;

	int channels;
	int mode;
	uint8_t * pixeldata;
} SpreadTexture;

SpreadTexture * SpreadCreateTexture( Spreadgine * spr, int x, int y, int chan, int mode );
SpreadTexture * SpreadLoadTexture( Spreadgine * spr, const char * tfile );
void SpreadApplyTexture( SpreadTexture * tex, int slot );
*/

#endif


