#ifndef _TEXTBOXES_H
#define _TEXTBOXES_H

#include <spreadgine_util.h>


//Double-up a terminal + textbox
typedef struct TextBox TextBox;
typedef struct TextBoxSet TextBoxSet;

struct TermStructure;

struct TextBox
{
	BatchedObject * obj;	//TextBox inherits all properties from BatchedObject.
	struct TermStructure * ts;
	struct TextBoxSet * parent;

	//Location of table in texture.
	int table_x, table_y;
	int width, height;		//In characters.
	int last_curx, last_cury;
	int last_scrollback;
	TextBox * next;
};

struct TextBoxSet
{
	BatchedSet * set;		//TextBoxSet inherits all properties from TextBox.
	SpreadGeometry * geo;	//Geometry template for text box.
	SpreadShader * shd;

	//Location of font in texture.
	int charset_x, charset_y, charset_w, charset_h;
	int font_w, font_h;

	TextBox * first;
};

//For fontfile, try "cntools/vlinterm/ibm437.pgm"
TextBoxSet * CreateTextBoxSet( Spreadgine * spr, const char * fontfile, int max_boxes, int texsizew, int texsizeh );
void RenderTextBoxSet( TextBoxSet * set, float * matrix_base );
void FreeTextBoxSet( TextBoxSet * set );

TextBox    * CreateTextBox( TextBoxSet * set, const char * name, int chars_w, int chars_h );
void		TextBoxUpdateExtraVertexData( TextBox * tb );
int			ResizeTextBox( TextBox * tb, int new_chars_w, int new_chars_h );
void        WriteToTextBox( TextBox * tb, int character );


#endif

