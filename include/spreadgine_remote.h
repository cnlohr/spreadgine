#ifndef _SPREADGINE_H_
#define _SPREADGINE_H

#include <spreadgine.h>

/////////////////////////////CORE TO-BROWSER///////////////////////
//
//Messages: [message size (Does not include opcode)] [opcode] [payload (optional)]
//	64 range
//		64 = InitializationPacket
//		65 = UpdateCameraName( uint8_t id, char name[...] ); //XXX TODO
//		66 = UpdateCameraPerspective( uint8_t id, float perspective[16] ); //XXX TODO
//		67 = UpdateCameraView( uint8_t id, float view[16] ); //XXX TODO
//		68 = SetupCamera( uint8_t camid, float fov, float aspect, float near, float far, [char*]camname );
//		69 = Setup New Shader (or update existing) ... See spreadgine_remote.c for mroe info.
//		70 = Remove Shader( uint8_t shader id );
//	
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
//		83..86 reserved for other uniform operations.

//		87 = Create new geometry (complicated fields, read in spreadgine.c)
//		88 = PushNewArrayData( uint8_t geono, int arrayno, [VOID*] data);
//		89 = SpreadRenderGeometry( uint8_t geono, int offset_at, int nr_verts, float modelmatrix[16] );
//		90 = RemoveGeometry( uint8_t geono );	//Tricky: There is no call to remove children of geometries.  Client must do that.
//		
//
//  128+ = User functions

//"entry" is in printf mode... "format" is in 'b' for byte, 'i' for integer, 'f' for float, 's' for string, 'v' takes two parameters, a # of bytes and a pointer to the payload. 'S' takes a # of strings and a list of strings.
//If entry is null, it is not stored to the hash table.
void SpreadMessage( Spreadgine * e, const char * entry, const char * format, ... ); //XXX WARNING Not threadsafe. ALSO "entry" IS NOT printf-compliant.  Use '#' to pull the next int.
void SpreadHashRemove( Spreadgine * e, const char * he, ... );
int SpreadCreateDump( Spreadgine * spr, uint8_t ** ptrout );

//Deploy for immediate delivery - don't use this for complicated operations or anything that needs to be cached.
void SpreadPushMessage( Spreadgine * e, uint8_t messageid, int payloadsize, void * payload );

//Internal, called only by spreadgine.
void * SpreadHTTPThread( void * spread );

void SpreadRemoteInit( Spreadgine * e );


//Don't call this unless you've already locked the common data mutex.
struct SpreadHashEntry * SpreadHashEntryGetOrInsert( Spreadgine * e , const char * he );


#endif


