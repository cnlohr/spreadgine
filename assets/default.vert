attribute vec3 vpos;
attribute vec4 vcolor;
uniform mat4 pmatrix, vmatrix, mmatrix;
varying vec4 vvColor;
varying vec4 vvPos;

uniform vec4 timevec;

void main()
{
    vvColor = vcolor;

    gl_Position = (pmatrix * 
		(vmatrix * 
		(mmatrix * vec4(vpos, 1.0))
	));

	vvPos = (mmatrix * vec4(vpos, 1.0));

	vec2 rscreenpos = gl_Position.xy/gl_Position.w;
	//Do wacky stuff to screenpos here if you want.
	gl_Position.xy = rscreenpos * gl_Position.w;

//	if(gl_Position.x/gl_Position.w>=.99) gl_Position.x=1./0.; //XXX Tricky: Discard polys if they would encroach...
}
