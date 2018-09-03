#ifndef _SPREADGINE_UTIL_H
#define _SPREADGINE_UTIL_H


#include <spreadgine.h>

//Immediate mode/dynamic mesh functionality
SpreadGeometry * CreateMeshGen( Spreadgine * spr, const char * geoname, int render_type, int max_iset );
void StartImmediateMode( SpreadGeometry * geo );
int ImmediateModeMesh( struct SpreadGeometry * geo, const float * trans44, const float * coloroff, const float * colorscale, const float * tcoff, const float * tcscale );
void UpdateMeshToGen( SpreadGeometry * geo );




SpreadGeometry * LoadOBJ( Spreadgine * spr, const char * filename, int flipv, int make_wireframe );
SpreadGeometry * MakeSquareMesh( Spreadgine * e, int w, int h );


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Batched operations

struct BatchedObject;
struct BatchedSet;
typedef struct BatchedObject BatchedObject;
typedef struct BatchedSet BatchedSet;

struct BatchedObject
{
	int which_index_place;
	int which_vertex_place;

	int vertices;
	int indices;

	int objinparent;
	char * name;
	BatchedSet * parent;

	struct ExtraTexs
	{
		int x, y;
		struct ExtraTexs * next;
	} * extratex;
};

struct Spatialloc;

//This structure gets crammed in the "user" pointer.
struct BatchedSet
{
	Spreadgine * spr;
	const char * setname;

	//Both arrays.
	uint8_t * allocated_indices; 	//Map, for whether a specific index value is allocated.  Note: First allocation is 1 further allocations are 2
	uint8_t * allocated_vertices;	//Map for whether a specific vertex is allocated.  Follows same rules as allocated_indices.

	SpreadTexture * associated_texture;
	struct Spatialloc * spatial_allocator;
	SpreadGeometry * coregeo;

	//Array of objects. NULL indicates unused.
	BatchedObject ** objects;

	uint8_t * internal_mbuffer; //For texture
	int internal_w;
	int internal_h;
	int tex_dirty;		//XXX BIG NOTE: If this is -1, caching is disabled.
	int geo_dirty;		//Same as above.
	int px_per_xform;

	int max_index;
	int max_objects;

	int highest_index;
	int highest_vertex;
};


//NOTE: px_per_xform MUST be at least 4, they are used specificially to hold "extra" data if exceeding 4.
BatchedSet * CreateBatchedSet( Spreadgine * spr, const char * setname, int max_objects, int max_indices, int render_type , int texturex, int texturey, int px_per_xform );
void FreeBatchedSet( BatchedSet * set );
void RenderBatchedSet( BatchedSet * set, SpreadShader * shd, const float * modelmatrix );

BatchedObject * AllocateBatchedObject( BatchedSet * set, SpreadGeometry * object, const char * name );
void UpdateBatchedObjectTransformData( BatchedObject * o, const float * Position, const float scale, const float * Quaternion, const float * extra );
void FreeBatchedObject( BatchedObject * o );
int  AllocateBatchedObjectTexture( BatchedObject * o, int * tx, int * ty, int w, int h );
int  FreeBatchedObjectTexture( BatchedObject * o, int tx, int ty );

//Storage of transform data in texture:
//  (Uses 4x1 pixels)
//  POS  MSB [x y z scale] POS  QUAT MSB [x y z w]     QUAT LSB [x y z w] LSB [x y z scale] 
//  

#endif


