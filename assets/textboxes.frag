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

/*  //For filtering
	vec3 fgcolor = vec3( tvB.yzw );
	vec3 bgcolor = vec3( tvA.zw, tvB.x );
	vec3 fg = finalchartex * fgcolor;
	vec3 bg = (1.-finalchartex) * bgcolor; ///??? Why is this faster than "mix"??
	gl_FragColor = vec4( fg+bg, 1.0 );*/

#if 0
	if( finalchartex > 0.5 )
		gl_FragColor = vec4( tvB.yzw, 1.0 );
	else
	{
		if( length( tvA.zw ) < .4 )
			discard;
		else
			gl_FragColor = vec4( tvA.zw, tvB.x, 1.0 );
	}

#else
	//For otherwise...

	if( finalchartex > 0.5 )
		gl_FragColor = vec4( tvB.yzw, 1.0 );
	else
		gl_FragColor = vec4( tvA.zw, tvB.x, 1.0 );
#endif

}
