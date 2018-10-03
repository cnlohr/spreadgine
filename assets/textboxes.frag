#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;	// [x, y] [scaled x, scaled y];  vv1Col.zw = vv1Col.xy * vvExtra.zw / batchsetpass.yw
varying vec4 vvExtra;	//coded text location in texture [x y] [w h]

uniform vec4 fontspot;	//Location within the texture of the font [x y] [w h] Only useful when font doesn't take full-room.


uniform sampler2D texture0, texture1, texture2;
//uniform vec4 batchsetuni; //invw, 1.0/set->associated_texture->w, set->px_per_xform * 0.5 * invw, 1.0/set->associated_texture->h
varying vec4 batchsetpass;


void main()
{
	//This linearizes the position we're getting the source text from over the space.
	//I.E. this can look into the array of the text buffer.

	vec2 fvpos = vv1Col.xy * vvExtra.zw + vvExtra.xy;

	vec4 tvA  = texture2D( texture0, fvpos );
	vec4 tvB  = texture2D( texture0, fvpos + vec2( 0, vvExtra.w ) ); //Doing the second texture look up "looks" slower but is in fact way faster, up from 25 FPS to 42.

	vec2 targetc = tvA.xy;
	vec2 placeincharacter = mod( vv1Col.zw, 1.0 ) / 16.0;
	targetc = targetc + placeincharacter - vec2( 0.005); //This tweaks the position in the texture we're looking.


	float finalchartex = (texture2D( texture1, targetc )).r; //Look up texture and color-stretch.

	vec3 fgcolor = vec3( tvB.yzw );
	vec3 bgcolor = vec3( tvA.zw, tvB.x );
	vec3 fg = finalchartex * fgcolor;
	vec3 bg = (1.-finalchartex) * bgcolor; ///??? Why is this faster than "mix"??
	gl_FragColor = vec4( fg+bg, 1.0 );
#if 0
	//Uncomment for more debug.
	//	    gl_FragColor = vec4( tv.xy*1.0, 0.1, 1.0); return;

	//Pointer to where in the font we need to look up.

	vec2 targetc = floor( vec2( mod( tv.x, 16.0 ), vec2( tv.x / 16.0 ) ) )/16.0;

	//targetc now contains 0..1, 0..1 of where to look up in the output map.



	gl_FragColor = finalchartex;

#if 1

//	finalchartex = clamp( (finalchartex - 0.4 ) * 2.0, 0.0, 1.0 );  Don't blur when close.
//	finalchartex = clamp( (finalchartex - 0.4 ) * 2.0, 0.0, 1.0 );  //<<This line makes it look good if you were doing biliner or trilinear interpolation.

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
#endif
#endif

}
