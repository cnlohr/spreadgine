#include "textboxes.h"
#include "../cntools/vlinterm/vlinterm.h"
#include "../src/spatialloc.h"
#include <string.h>

struct TermStructure;

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

	//Add uniform pointing to font table.
	int slot = SpreadGetUniformSlot( tb->shd, "fontspot" );
	if( slot >= 0 )
	{
		float ssf[4] = {
			0.0 / (float)ths->associated_texture->w,
			0.0 / (float)ths->associated_texture->h, 
			( tb->charset_w ) / (float)tb->charset_w_exp, 
			( tb->charset_h ) / (float)tb->charset_h_exp, 
		};
		SpreadUniform4f( tb->shd, slot, ssf );
	}
	else
	{
		//XXX TODO: Add a "Warning" system.
		//fprintf( stderr, "Error: Can't find parameter in shader\n" );
	}

}


TextBoxSet * CreateTextBoxSet( Spreadgine * spr, const char * fontfile, int max_boxes, int texsizew, int texsizeh )
{
	SpreadGeometry * geo = MakeSquareMesh( spr, 30, 3 );
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

		#if 0		//Allow non-power-of-two textures?
		charset_w_exp = charset_w;
		charset_h_exp = charset_h;

		charset_texture = SpreadCreateTexture( spr, fontfile, charset_w_exp, charset_h_exp, 4, GL_UNSIGNED_BYTE );

//		SpreadChangeTextureProperties( charset_texture, 2, 1, 2 );  For mipmaps
		SpreadChangeTextureProperties( charset_texture, 1, 1, 0 );

		SpreadUpdateSubTexture( charset_texture, retfont, 0, 0, charset_w_exp, charset_h_exp );
		free( retfont );

		#else
		//We do a little work to make sure we have a power-of-two texture.  This code is untested and may not be needed.

		charset_w_exp = pow2roundup( charset_w );
		charset_h_exp = pow2roundup( charset_h );
		charset_texture = SpreadCreateTexture( spr, fontfile, charset_w_exp, charset_h_exp, 4, GL_UNSIGNED_BYTE );
		SpreadChangeTextureProperties( charset_texture, 2, 0, 1, 2 );
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

		//We do some trickery here to only update the portion of the screen that's changed.
		if( t->last_curx != ts->curx || t->last_cury !=  ts->cury || ts->tainted || taint_all )
		{
			int x, y;
			int extentminy = h;
			int extentmaxy = 0;

			if( t->last_cury != ts->cury ) {
				extentminy = (ts->cury < t->last_cury)?ts->cury:t->last_cury;
				extentmaxy = (ts->cury > t->last_cury)?ts->cury:t->last_cury;
				if( extentmaxy >= h ) extentmaxy = h-1;		
				if( extentminy >= h ) extentminy = h-1;		
			}

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
					ts->linetaint[i] = 0;
					if( i < extentminy ) extentminy = i;
					if( i > extentmaxy ) extentmaxy = i; 
				}
			}
			//Why does this happen?  It definitely does.  When it does things crash.
			//Further investigation should be made as to what's going on there...
			if( extentmaxy < extentminy ) { int stop = extentmaxy; extentmaxy = extentminy; extentminy = stop; } 


			if( extentmaxy >= h ) extentmaxy = h-1;
			if( extentminy < 0 ) extentminy = 0;

			int updatelines = extentmaxy - extentminy + 1;
			uint32_t texo[w * updatelines];

			ts->tainted = 0;	//Taint up here in case the buffer changes while we're updating.
			//If something changed before here, it's ok because it's already read.

			if( t->last_curx < w && t->last_cury < h )				tb[t->last_curx + t->last_cury * w] |= 1<<24;
			if( ts->curx < w && ts->cury < h )						tb[ts->curx + ts->cury * w] |= 1<<24;
			t->last_curx = ts->curx;
			t->last_cury = ts->cury;

			memcpy( texo, tb + extentminy * w, updatelines * 4 * w );

			int cursorplace = (ts->curx + (ts->cury-extentminy)*w);
			if( cursorplace >= 0 && cursorplace < w*updatelines )
				texo[cursorplace] |= 0x00ffff00; //

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
		DestroyTextBox( tt );
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
	ret->last_curx = -1;
	ret->last_cury = -1;
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

void DestroyTextBox( TextBox * tb )
{
	close( tb->ts->ptspipe );
	OGJoinThread( tb->rxthread );
	free( tb );
}


static void * rxthread( void * v )
{
	struct TermStructure * ts = v;

    while( 1 )
	{
		uint8_t rxdata[1024+1];
		int rx = read( ts->ptspipe, rxdata, 1024 );
		if( rx < 0 ) break;
		int i;
		for( i = 0; i < rx; i++ )
		{
			char crx = rxdata[i];
			EmitChar( ts, crx );
		}
	}
	fprintf( stderr, "Error: Terminal pipe died\n" ); 
	close( ts->ptspipe );
	ts->ptspipe = 0;
	return 0;
}


int TextboxAttachTerminal( TextBox * tb, char * const *  localargv )
{
	int r = tb->ts->ptspipe = spawn_process_with_pts( "/bin/bash", localargv, &tb->ts->pid );
	ResizeScreen( tb->ts, tb->ts->charx, tb->ts->chary );
	tb->rxthread = OGCreateThread( rxthread, (void*)tb->ts );
	return r;
}

void TextBoxHandleKeyX11( TextBox * tb, int keycode, int bDown )
{
	struct TermStructure * ts = tb->ts;

	static int shift_held;
	static int ctrl_held;
	static int alt_held;

	if( keycode == 65506 || keycode == 65505 )
	{
		shift_held = bDown;
	}
	else if( keycode == 65507 )
	{
		ctrl_held = bDown;
	} 
	else if( keycode == 65513 )
	{
		alt_held = bDown;
	}
	else if( bDown )
	{
		if( shift_held && keycode == 65366 )
		{
			TermScroll( ts, -ts->chary/2 );
		}
		else if( shift_held && keycode == 65365 )
		{
			TermScroll( ts, ts->chary/2 );
		}
		else
		{
			int len = 0;
			const char * str = 0;
			char cc[3] = { keycode };

			if( keycode == 65293 || keycode == 65421 )
			{
				//Enter key: Be careful with these...
				cc[0] = '\x0d';
				len = 1;
				if( ts->dec_private_mode & (1<<20) )
				{
					cc[1] = '\x0a';
					len = 2;
				}
				str = cc;
				ts->scrollback = 0;
			}
			else if( keycode >= 255 )
			{
				struct KeyLooup
				{
					unsigned short key;
					short stringlen;
					const char * string;
				};
				const struct KeyLooup keys[] = {
					{ 65362, 3, "\x1b[A" },  //Up
					{ 65364, 3, "\x1b[B" },  //Down
					{ 65361, 3, "\x1b[D" },  //Left
					{ 65363, 3, "\x1b[C" },  //Right
					{ 65288, 1, "\x08" },
					{ 65289, 1, "\x09" },
					{ 65307, 1, "\x1b" },
					{ 65366, 4, "\x1b[6~" }, //PgDn
					{ 65365, 4, "\x1b[5~" }, //PgUp
					{ 65367, 3, "\x1b[F" },  //Home
					{ 65360, 3, "\x1b[H" },  //End
					{ 65535, 4, "\x1b[3~" }, //Del
					{ 65379, 4, "\x1b[4~" }, //Ins
					{ 255, 0, "" },
				};
				int i;
				for( i = 0; i < sizeof(keys)/sizeof(keys[0]); i++ )
				{
					if( keys[i].key == keycode )
					{
						len = keys[i].stringlen;
						str = keys[i].string;
						break;
					}
				}
				if( i < 4 && ( ts->dec_private_mode & 2 ) ) 			//Handle DECCKM, not sure why but things like alsamixer require it.
				{
					cc[0] = '\x1b';
					cc[1] = 'O';
					cc[2] = str[2];
					str = cc;
				}
				if( i == sizeof(keys)/sizeof(keys[0]) ) fprintf( stderr, "Unmapped key %d\n", keycode );
			} else
			{
				extern int g_x_global_key_state;
				extern int g_x_global_shift_key;
				if( (g_x_global_key_state & 2) && !(g_x_global_key_state & 2) )
				{
					if( keycode >= 'a' && keycode <= 'z' )
						keycode = g_x_global_shift_key;
				}
				else if( g_x_global_key_state & 1 )
				{
					keycode = g_x_global_shift_key;
				}
				else if( ctrl_held )
				{
					if( keycode >= 'a' && keycode <= 'z' )
					{
						keycode = keycode - 'a' + 1;
					}
					else if( keycode == ']' )
					{
						keycode = 0x1d;
					}
				}
				cc[0] = keycode;
				str = cc;
				len = 1;
			}

			int i;
			for( i = 0; i < len; i++ )
			{
				FeedbackTerminal( ts, str + i, 1 );
				if( ts->echo ) EmitChar( ts, str[i] );
			}
		}
	}
}
