attribute vec3 attrib0;
attribute vec4 attrib1;
uniform mat4 pmatrix, vmatrix, mmatrix;
varying vec4 vvColor;
varying vec4 vvPos;

uniform vec4 timevec;

void main()
{
    vvColor = vec4(attrib1.xyz, 1.0 );

    gl_Position = (pmatrix * 
		(vmatrix * 
		(mmatrix * vec4(attrib0, 1.0))
	));

	vvPos = (mmatrix * vec4(attrib0, 1.0));

	vec2 rscreenpos = gl_Position.xy/gl_Position.w;

	//My awful method for trying to correct for lens warp.
	float compb = dot(rscreenpos,rscreenpos);
	compb = min( compb, 2.0 );
	rscreenpos *= (1.0-  compb*.1);

	//vvColor.rgb = vec3( vec2(compb), 1.0 );
	gl_Position.xy = rscreenpos * gl_Position.w;

//Only needed on orange pi
//	float divx = gl_Position.x/gl_Position.w;
//	if(divx>=.99)  gl_Position.x=gl_Position.w*.99; //XXX Tricky: Discard polys if they would encroach...
//	if(divx<=-.99) gl_Position.x=gl_Position.w*-.99; //XXX Tricky: Discard polys if they would encroach...
}
