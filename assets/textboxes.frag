#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;	//coded text location in texture [x y] [w h]
//uniform vec4 fontspot;	//Location within the texture of the font [x y] [w h]
uniform sampler2D texture0, texture1;
uniform vec4 batchsetuni; //invw, 1.0/set->associated_texture->w, set->px_per_xform * 0.5 * invw, 1.0/set->associated_texture->h


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

	vec2 char_area_size = vvExtra.zw / batchsetuni.yw;
	vec2 placeincharacter = mod( vv1Col.xy * char_area_size, 1.0 ) / 16.0;

	vec2 targetc = trunc( vec2( mod( tv.x, 16.0 ), tv.x / 16.0 ) )/16.0;
	//targetc now contains 0..1, 0..1 of where to look up in the output map.

	targetc = targetc + placeincharacter + vec2( -0.01, 0.0001 );

	vec4 finalchartex = (texture2D( texture1, targetc )-0.1)*2.0;
	gl_FragColor = vec4( finalchartex.xyz, 1.0 );


#if 0

#ifdef NOTERP
	vec4 finalchartex = texture2D( texture0, targetc  * fontspot.zw + fontspot.xy );
	gl_FragColor = vec4( finalchartex.xy, 0.0, 1.0 );
#else
	vec2 ttargetc = (targetc-0.0042)  * fontspot.zw + fontspot.xy;
	vec4 finalchartex0 = texture2D( texture0, ttargetc );
	vec4 finalchartex1 = texture2D( texture0, ttargetc + vec2( batchsetuni.y, 0.0 ) );
	vec4 finalchartex2 = texture2D( texture0, ttargetc + vec2( 0.0, batchsetuni.w ) );
	vec4 finalchartex3 = texture2D( texture0, ttargetc + batchsetuni.yw );
	vec2 mixing = mod( ttargetc / batchsetuni.yw, 1.0 );
	vec4 mixx0 = mix( finalchartex0, finalchartex1, mixing.x );
	vec4 mixx1 = mix( finalchartex2, finalchartex3, mixing.x );
	vec4 finalchartexo = mix( mixx0, mixx1, mixing.y );

	gl_FragColor = vec4( (finalchartexo.xy-0.5)*4.0, 0.0, 1.0 );

#endif
#endif
}
