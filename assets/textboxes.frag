#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;	//coded text location in texture [x y] [w h]
uniform vec4 fontspot;	//Location within the texture of the font [x y] [w h]
uniform sampler2D texture0;
uniform vec4 batchsetuni; //invw, 1.0/set->associated_texture->w, set->px_per_xform * 0.5 * invw, 1.0/set->associated_texture->w


void main()
{
//	gl_FragColor = texture2D ( texture0, vv1Col.xy * fontspot.zw + fontspot.xy); return;

	//This linearizes the position over the space.
	vec2 fvpos = vv1Col.xy * vvExtra.zw + vvExtra.xy;

	vec4 tv   = texture2D( texture0, fvpos ) * 255.0;
	//'tv' now contains the vlinterm structure data.
		// text  <<lsB
		// attrib
		// color
		// taint <<msB  (Could also contain more attirbutes if needed)

	//Uncomment for more debug.
	//    gl_FragColor = vec4( tv.xy*1.0, 0.1, 1.0); return;

	//Pointer to where in the font we need to look up.
	vec2 targetcell = trunc( vec2( mod( tv.x, 16.0 ), tv.x / 16.0 ) ) / 16.0 * fontspot.zw + fontspot.xy;



	//The X and Y dimensions of the text area.
	vec2 char_area_size = vvExtra.zw / batchsetuni.yw;

	vec2 placeincharacter = mod( vv1Col.xy, 1.0 ) / 16.0 + targetcell;
	placeincharacter = placeincharacter * fontspot.zw + fontspot.xy;
	vec4 finalchartex = texture2D( texture0, placeincharacter );

	gl_FragColor = vec4( finalchartex.xy*4.0, 0.0, 1.0 );
}
