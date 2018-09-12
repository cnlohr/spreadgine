#include "textboxes.h"
#include "../cntools/vlinterm/vlinterm.h"
#include "../src/spatialloc.h"
#include <string.h>

struct TermStructure;

static int LoadFont( const char * fontfile, int * charset_w, int * charset_h, int * font_w, int * font_h, uint32_t ** retfont )
{

	FILE * f = fopen( fontfile, "rb" );
	int c = 0, i;
	int format;
	if( !f ) { fprintf( stderr, "Error: cannot open font file %s\n", fontfile ); return -1; }
	if( (c = fgetc(f)) != 'P' ) { fprintf( stderr, "Error: Cannot parse first line of font file [%d].\n", c ); return -2; } 
	format = fgetc(f);
	fgetc(f);
	while( (c = getc(f)) != EOF ) { if( c == '\n' ) break; }	//Comment
	if( fscanf( f, "%d %d\n", charset_w, charset_h ) != 2 ) { fprintf( stderr, "Error: No size in pgm.\n" ); return -3; }
	while( (c = getc(f)) != EOF ) if( c == '\n' ) break;	//255
	if( (*charset_w & 0x0f) || (*charset_h & 0x0f) ) { fprintf( stderr, "Error: charset must be divisible by 16 in both dimensions.\n" ); return -4; }
	uint32_t * font = *retfont = malloc( *charset_w * *charset_h * 4 );

	for( i = 0; i < *charset_w * *charset_h; i++ ) { 
		if( format == '5' )
		{
			c = getc(f);
			font[i] = c | (c<<8) | (c<<16) | (0xff000000);
		}
		else
		{
			fprintf( stderr, "Error: unsupported font image format.  Must be P5\n" );
			return -5;
		}
	}
	*font_w = *charset_w / 16; //16 chars wide
	*font_h = *charset_h / 16; //16 chars high
	fclose( f );

	return 0;
}

static void text_update_uniform_callback( struct BatchedSet * ths )
{
	struct TextBoxSet * tb = ths->user;
	SpreadApplyTexture( tb->charset_texture, 1 );

/*
	//Add uniform pointing to font table.
	int slot = SpreadGetUniformSlot( tb->shd, "fontspot" );
	if( slot >= 0 )
	{
		float ssf[4] = {
			tb->charset_x / (float)ths->associated_texture->w,
			tb->charset_y / (float)ths->associated_texture->h, 
			( tb->charset_w ) / (float)ths->associated_texture->w, 
			( tb->charset_h ) / (float)ths->associated_texture->h, 
		};
		SpreadUniform4f( tb->shd, slot, ssf );
	}
	else
	{
		//XXX TODO: Add a "Warning" system.
		//fprintf( stderr, "Error: Can't find parameter in shader\n" );
	}
*/
}

static unsigned pow2roundup( unsigned v )
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

TextBoxSet * CreateTextBoxSet( Spreadgine * spr, const char * fontfile, int max_boxes, int texsizew, int texsizeh )
{
	SpreadGeometry * geo = MakeSquareMesh( spr, 3, 3 );
	const int index_needed_per_box = (geo->verts>geo->indices)?geo->verts:geo->indices;

	int charset_w, charset_h, font_w, font_h;
	int charset_w_exp, charset_h_exp;
	SpreadTexture * charset_texture;
	{
		uint32_t * retfont;
		if( LoadFont( fontfile, &charset_w, &charset_h, &font_w, &font_h, &retfont ) )
		{
			fprintf( stderr, "Error loading font for text boxes.\n" );
			SpreadFreeGeometry( geo );
			return 0;
		}

		#if 1
		charset_w_exp = charset_w;
		charset_h_exp = charset_h;

		charset_texture = SpreadCreateTexture( spr, fontfile, charset_w_exp, charset_h_exp, 4, GL_UNSIGNED_BYTE );

		SpreadChangeTextureProperties( charset_texture, 2, 1, 2 );

		SpreadUpdateSubTexture( charset_texture, retfont, 0, 0, charset_w_exp, charset_h_exp );
		free( retfont );

		#else
		//We do a little work to make sure we have a power-of-two texture.  This code is untested and may not be needed.

		charset_w_exp = pow2roundup( charset_w );
		charset_h_exp = pow2roundup( charset_h );
		charset_texture = SpreadCreateTexture( spr, fontfile, charset_w_exp, charset_h_exp, 4, GL_UNSIGNED_BYTE, 2, 1 );

		//Build an expanded image for loading into the texture.
		uint32_t * fontup = malloc( charset_w_exp * charset_h_exp * 4 );
		int x, y;
		for( y = 0; y < charset_h_exp; y++ )
		for( x = 0; x < charset_w_exp; x++ )
		{
			if( x < charset_w && y < charset_h )
				fontup[x+y*charset_w_exp] = retfont[x+y*charset_w];
			else
				fontup[x+y*charset_w_exp] = 0;
		}

		free( retfont );
		SpreadUpdateSubTexture( charset_texture, fontup, 0, 0, charset_w_exp, charset_h_exp );
		free( fontup );
		#endif

	}

	BatchedSet * set = CreateBatchedSet( spr, "textboxes", max_boxes, index_needed_per_box * max_boxes, GL_TRIANGLES, texsizew, texsizeh, 6 ); //6: Need one set of extra attributes.
	if( !set )
	{
		fprintf( stderr, "Error: Can't create batch for textboxes.\n" );
		SpreadFreeGeometry( geo );
		SpreadFreeTexture( charset_texture );
		return 0;
	}

	TextBoxSet * ret = calloc( sizeof( TextBoxSet ), 1 );
	set->user = ret;
	set->update_uniform_callback = text_update_uniform_callback;
	ret->set = set;
	ret->geo = geo;

//	int spa = SpatMalloc( set->spatial_allocator, charset_w, charset_h, &ret->charset_x, &ret->charset_y );
	SpreadShader * shd = SpreadLoadShader( spr, "text_shd", "assets/textboxes.frag", "assets/textboxes.vert", 0 );
	if( !shd )
	{
		fprintf( stderr, "Aborting text box set [%p]\n", shd );
		FreeBatchedSet( set );
		SpreadFreeTexture( charset_texture );
		if( shd ) SpreadFreeShader( shd );
		SpreadFreeGeometry( geo );
		free( ret );
		return 0;
	}

	ret->shd = shd;
	ret->charset_w = charset_w;
	ret->charset_h = charset_h;
	ret->charset_w_exp = charset_w_exp;
	ret->charset_h_exp = charset_h_exp;
	ret->charset_texture = charset_texture;
	ret->first = 0;

	//SpreadUpdateSubTexture( set->associated_texture, retfont, ret->charset_x, ret->charset_y, charset_w, charset_h );

	return ret;
}



void RenderTextBoxSet( TextBoxSet * set, float * matrix_base )
{
	TextBox * t = set->first;
	while( t )
	{
		struct TermStructure * ts = t->ts;
		uint32_t * tb = ts->termbuffer;


		int h = ts->chary;
		int w = ts->charx;

		int scrollback = ts->scrollback;
		if( scrollback >= ts->historyy-h ) scrollback = ts->historyy-h-1;
		if( scrollback < 0 ) scrollback = 0;
		int taint_all = scrollback != t->last_scrollback;


		if( t->last_curx != ts->curx || t->last_cury !=  ts->cury || ts->tainted || taint_all )
		{
			int x, y;
			int extentminy = h;
			int extentmaxy = 0;

			if( t->last_cury != ts->cury ) { extentminy = extentmaxy = ts->cury; }

			if( taint_all )
			{
				extentminy = 0;
				extentmaxy = h;
			}
			else
			{
				int i;
				for( i = 0; i < h; i++ )
				{
					if( !ts->linetaint[i] ) continue;
					if( i < extentminy ) extentminy = i;
					if( i > extentmaxy ) extentmaxy = i; 
				}
			}

			int updatelines = extentmaxy - extentminy + 1;

			uint32_t texo[w * updatelines];

			ts->tainted = 0;	//Taint up here in case the buffer changes while we're updating.
			if( t->last_curx < w && t->last_cury < h )				tb[t->last_curx + t->last_cury * w] |= 1<<24;
			if( ts->curx < w && ts->cury < h )						tb[ts->curx + ts->cury * w] |= 1<<24;
			t->last_curx = ts->curx;
			t->last_cury = ts->cury;

			memcpy( texo, tb + extentminy * 4 * w, updatelines * 4 * w );

			SpreadUpdateSubTexture( t->parent->set->associated_texture, texo, t->table_x, t->table_y + extentminy, w, updatelines );
			t->last_scrollback = scrollback;
		}
		t = t->next;
	}

	RenderBatchedSet( set->set, set->shd, matrix_base );
}

void FreeTextBoxSet( TextBoxSet * set )
{
	FreeBatchedSet( set->set );
	SpreadFreeGeometry( set->geo );
	SpreadFreeShader( set->shd );
	SpreadFreeTexture( set->charset_texture );

	TextBox * tb = set->first;
	while( tb )
	{
		TextBox * tt = tb;
		//Do stuff, like update the textures, etc.
		tb = tb->next;
		free( tt );
	}

}

void TextBoxUpdateExtraVertexData( TextBox * tb )
{
	BatchedObject * o = tb->obj;
	const float Extras[4] = {
		tb->table_x / (float)o->parent->associated_texture->w,
		tb->table_y / (float)o->parent->associated_texture->h,
		tb->width   / (float)o->parent->associated_texture->w,
		tb->height  / (float)o->parent->associated_texture->h,
	};
	UpdateBatchedObjectTransformData( o, 0, 0, Extras );
}


TextBox    * CreateTextBox( TextBoxSet * set, const char * name, int chars_w, int chars_h )
{
	TextBox * ret;

	BatchedObject * obj = AllocateBatchedObject( set->set, set->geo, name );

	if( !obj )
	{
		fprintf( stderr, "Error: Couldn't instanciate new textbox\n" );
		return 0;
	}

	int tx, ty;
	int allc = AllocateBatchedObjectTexture( obj, &tx, &ty, chars_w, chars_h );
	if( allc )
	{
		fprintf( stderr, "Error: couldn't get enough texture space to make textbox\n" );
		FreeBatchedObject( obj );
		return 0;
	}

	ret = malloc( sizeof( TextBox ) );
	ret->obj = obj;
	ret->parent = set;

	struct TermStructure * ts = ret->ts = calloc( sizeof( struct TermStructure ), 1 );
	ts->screen_mutex = OGCreateMutex();
	ts->charx = chars_w;
	ts->chary = chars_h;
	ts->echo = 0;
	ts->historyy = 1000;
	ts->termbuffer = 0;
	ResetTerminal( ret->ts );

	ret->ts->user = ret;
	ret->width = chars_w;
	ret->height = chars_h;
	ret->table_x = tx;
	ret->table_y = ty;
	ret->last_scrollback = 0;

	//Insert the linked list object.
	ret->next = set->first;
	set->first = ret;

	TextBoxUpdateExtraVertexData( ret );

	return ret;
}


void HandleOSCCommand( struct TermStructure * ts, int parameter, const char * value )
{
	TextBox * tb = ts->user;
}

void HandleBell( struct TermStructure * ts )
{
	TextBox * tb = ts->user;
}


void         WriteToTextBox( TextBox * tb, int character )
{
	EmitChar( tb->ts, character );
}

int			ResizeTextBox( TextBox * tb, int new_chars_w, int new_chars_h )
{
	int tx, ty;
	printf( "RESIZE\n");
	int allc = AllocateBatchedObjectTexture( tb->obj, &tx, &ty, new_chars_w, new_chars_h );
	if( allc )
	{
		fprintf( stderr, "Error: can't reallocated texture %d, %d -> %d, %d\n", tb->width, tb->height, new_chars_w, new_chars_h );
		return -1;
	}

	FreeBatchedObjectTexture( tb->obj, tb->table_x, tb->table_y );

	tb->table_x = tx;
	tb->table_y = ty;

	tb->width = new_chars_w;
	tb->height = new_chars_h;

	TextBoxUpdateExtraVertexData( tb );

	return 0;
}


