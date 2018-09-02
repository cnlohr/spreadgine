attribute vec3 attrib0;
attribute vec4 attrib1;
attribute vec4 attrib2;
uniform mat4 pmatrix, vmatrix, mmatrix;
varying vec4 vv0Pos;
varying vec4 vv1Col;
varying vec4 vv2Tex;

uniform vec4 timevec;

attribute vec3 attrib0;
attribute vec4 attrib1;
attribute vec4 attrib2;
uniform sampler2D texture0;


void main()
{
	vv1Col = attrib1;
    vv2Tex = attrib2;
	vec2 gtx = attrib2.xy;
	vec4 tla = vec4(0.), tlb = vec4(0.);
	tla += texture2D( texture0, gtx + vec2( 0.001, 0. ) )*3.;
	tlb += texture2D( texture0, gtx + vec2( 0.002, 0.001 ) )*3.;
	tla += texture2D( texture0, gtx + vec2( 0.003, 0. ) )*3.;

	tlb += texture2D( texture0, gtx + vec2( 0.001, 0.001 ) )*3.;
	tla += texture2D( texture0, gtx + vec2( 0.004, 0. ) )*3.;
	tlb += texture2D( texture0, gtx + vec2( 0.003, 0.001 ) )*3.;
	tla += texture2D( texture0, gtx + vec2( 0.002, 0. ) )*3.;
	tlb += texture2D( texture0, gtx + vec2( 0.004, 0.001 ) )*3.;
	tla += texture2D( texture0, gtx + vec2( 0.001, 0. ) )*3.;
	tlb += texture2D( texture0, gtx + vec2( 0.002, 0.001 ) )*3.;
	tla += texture2D( texture0, gtx + vec2( 0.003, 0. ) )*3.;

	
	vec4 tlu = (tla+tlb)*.1;

    gl_Position = (pmatrix * 
		(vmatrix * 
		(mmatrix * vec4(attrib0+tlu.xyz, 1.0))
	));


	vv0Pos = (mmatrix * vec4(attrib0, 1.0));

	vec2 rscreenpos = gl_Position.xy/gl_Position.w;

	//My awful method for trying to correct for lens warp.
	float compb = dot(rscreenpos,rscreenpos);
	compb = min( compb, 2.0 );
	rscreenpos *= (1.0-  compb*.1);

	gl_Position.xy = rscreenpos * gl_Position.w;
}
