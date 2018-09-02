attribute vec3 attrib0;
attribute vec4 attrib1;
attribute vec4 attrib2;
uniform mat4 pmatrix, vmatrix, mmatrix;
varying vec4 vv0Pos;
varying vec4 vv1Col;
varying vec4 vv2Tex;

uniform vec4 timevec;

uniform sampler2D texture0;

const vec2 texsizeinv = vec2( 1./2048. );
const vec2 texadvance = vec2( 1./2048., 0.0 );

void main()
{
	vv1Col = attrib1;
    vv2Tex = attrib2;

	vec2 gtx = attrib2.xy;
	vec4 tpos = texture2D( texture0, gtx );
	gtx += texadvance;
	vec4 tquat = texture2D( texture0, gtx );
	gtx += texadvance;
	tpos += texture2D( texture0, gtx ) / 256.0;
	gtx += texadvance;
	tquat += texture2D( texture0, gtx ) / 256.0;

	//tpos = pos * 1000.0/?????
	tpos = tpos * (32768.0/1000.0) - 32767;
	tquat *= 32768.0/1000.0;

vec2( texsizeinv.x   texture2D( texture0, gtx + texsizeinv *  );

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
