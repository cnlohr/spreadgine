attribute vec3 v0pos;
attribute vec4 v1col;
attribute vec4 v2tex;
uniform mat4 pmatrix, vmatrix, mmatrix;
varying vec4 vv2Tex;
varying vec4 vv1Col;
varying vec4 vv0Pos;

uniform vec4 timevec;

void main()
{
    vv2Tex = v2tex;
	vv1Col = v1col;

    gl_Position = (pmatrix * 
		(vmatrix * 
		(mmatrix * vec4(v0pos, 1.0))
	));

	vv0Pos = (mmatrix * vec4(v0pos, 1.0));

	vec2 rscreenpos = gl_Position.xy/gl_Position.w;

	//My awful method for trying to correct for lens warp.
	float compb = dot(rscreenpos,rscreenpos);
	compb = min( compb, 2.0 );
	rscreenpos *= (1.0-  compb*.1);

	gl_Position.xy = rscreenpos * gl_Position.w;
}
