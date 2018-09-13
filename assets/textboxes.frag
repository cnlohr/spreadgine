#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;	//coded text location in texture [x y] [w h]

uniform vec4 fontspot;	//Location within the texture of the font [x y] [w h] Only useful when font doesn't take full-room.


uniform sampler2D texture0, texture1;
//uniform vec4 batchsetuni; //invw, 1.0/set->associated_texture->w, set->px_per_xform * 0.5 * invw, 1.0/set->associated_texture->h
varying vec4 batchsetpass;


void main()
{
//	gl_FragColor = texture2D ( texture0, vv1Col.xy * fontspot.zw + fontspot.xy); return;

	//This linearizes the position we're getting the source text from over the space.
	//I.E. this can look into the array of the text buffer.
	vec2 fvpos = vv1Col.xy * vvExtra.zw + vvExtra.xy;

	vec4 tv   = texture2D( texture0, fvpos ) * 255.5;
	//'tv' now contains the vlinterm structure data.
		// text  <<lsB
		// attrib
		// color
		// taint <<msB  (Could also contain more attirbutes if needed)

	//Uncomment for more debug.
	//    gl_FragColor = vec4( tv.xy*1.0, 0.1, 1.0); return;

	//Pointer to where in the font we need to look up.

	vec2 char_area_size = vvExtra.zw / batchsetpass.yw;
	vec2 placeincharacter = mod( vv1Col.xy * char_area_size, 1.0 ) / 16.0;

	vec2 targetc = floor( vec2( mod( tv.x, 16.0 ), vec2( tv.x / 16.0 ) ) )/16.0;
	//targetc now contains 0..1, 0..1 of where to look up in the output map.

	targetc = targetc + placeincharacter + vec2( -0.001, -0.001 ); //This tweaks the position in the texture we're looking.

	targetc *= fontspot.zw;

	vec4 finalchartex = (texture2D( texture1, targetc )); //Look up texture and color-stretch.

//	finalchartex = clamp( (finalchartex - 0.4 ) * 2.0, 0.0, 1.0 );  Don't blur when close.
	finalchartex = clamp( (finalchartex - 0.4 ) * 2.0, 0.0, 1.0 );

	//Get color + formatting.

		//Attrib & 4 == invert colors
		//Attrib & 1 == extra bold
	float intensity = (mod( tv.y, 2.0 ) >= 1.0)?1.0:0.7;
	if( mod(tv.y/16.0,2.0) >= 1.0 ) finalchartex.rgb = vec3(1.0)- finalchartex.rgb; 
	float red = (mod( tv.z, 2.0 ) >= 1.0)?intensity:0.0;
	float grn = (mod( tv.z/2.0, 2.0 ) >= 1.0)?intensity:0.0;
	float blu = (mod( tv.z/4.0, 2.0 ) >= 1.0)?intensity:0.0;

//	if( length(finalchartex.xyz) < 0.1 ) discard;

	finalchartex.rgb *= vec3( red, grn, blu );
	gl_FragColor = vec4( finalchartex.xyz+0.1, 1.0 );
}
