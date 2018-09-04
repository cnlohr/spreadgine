#include "textboxes.h"
#include "../cntools/vlinterm/vlinterm.h"
#include "../src/spatialloc.h"

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

TextBoxSet * CreateTextBoxSet( Spreadgine * spr, const char * fontfile, int max_boxes, int texsizew, int texsizeh )
{
	SpreadGeometry * geo = MakeSquareMesh( spr, 3, 3 );
	const int index_needed_per_box = (geo->verts>geo->indices)?geo->verts:geo->indices;


	int font_w, font_h;
	int charset_w, charset_h;
	uint32_t * retfont;
	if( LoadFont( fontfile, &charset_w, &charset_h, &font_w, &font_h, &retfont ) )
	{
		fprintf( stderr, "Error loading font for text boxes.\n" );
		SpreadFreeGeometry( geo );
		return 0;
	}

	BatchedSet * set = CreateBatchedSet( spr, "textboxes", max_boxes, index_needed_per_box * max_boxes, GL_TRIANGLES , texsizew, texsizeh, 6 ); //6: Need one set of extra attributes.
	if( !set )
	{
		fprintf( stderr, "Error: Can't create batch for textboxes.\n" );
		SpreadFreeGeometry( geo );
		free( retfont );
		return 0;
	}

	TextBoxSet * ret = malloc( sizeof( TextBoxSet ) );
	ret->set = set;
	ret->geo = geo;

	int spa = SpatMalloc( set->spatial_allocator, charset_w, charset_h, &ret->charset_x, &ret->charset_y );
	SpreadShader * shd = SpreadLoadShader( spr, "text_shd", "assets/textboxes.frag", "assets/textboxes.vert", 0 );
	if( spa || !shd )
	{
		fprintf( stderr, "Aborting text box set [%d %p]\n", spa, shd );
		free( retfont );
		FreeBatchedSet( set );
		if( shd ) SpreadFreeShader( shd );
		SpreadFreeGeometry( geo );
		free( ret );
		return 0;
	}

	ret->shd = shd;
	ret->font_w = font_w;
	ret->font_h = font_h;
	ret->charset_w = charset_w;
	ret->charset_h = charset_h;

	SpreadUpdateSubTexture( set->associated_texture, retfont, ret->charset_x, ret->charset_y, charset_w, charset_h );

	free( retfont );

	return ret;
}

void RenderTextBoxSet( TextBoxSet * set, float * matrix_base )
{
	//Add uniform pointing to font table.
	RenderBatchedSet( set->set, set->shd, matrix_base );
}

void FreeTextBoxSet( TextBoxSet * set )
{
	FreeBatchedSet( set->set );
	SpreadFreeGeometry( set->geo );
	SpreadFreeShader( set->shd );
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
	ret->ts = malloc( sizeof( struct TermStructure ) );
	ResetTerminal( ret->ts );
	ret->ts->user = ret;
	ret->width = chars_w;
	ret->height = chars_h;
	ret->table_x = tx;
	ret->table_y = ty;

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

	return 0;
}


