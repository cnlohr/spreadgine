attribute vec3 attrib0;
attribute vec4 attrib1;
attribute vec4 attrib2;
uniform mat4 pmatrix, vmatrix, mmatrix;

varying vec4 vv0Pos;
varying vec4 vv1Col;
varying vec4 vv2Tex;
varying vec4 vvExtra;

uniform vec4 timevec;

uniform sampler2D texture0;

const vec2 texsizeinv = vec2( 1./2048. );
const vec2 texadvance = vec2( 1./2048., 0.0 );

void main()
{
	vec3 vert = attrib0;
	vv1Col = attrib1;
    vv2Tex = attrib2;

	{
		vec4 textra;
		vec2 gtx = attrib2.zw;
		vec4 tpos = texture2D( texture0, gtx ) * 255.;
		gtx += texadvance;
		vec4 tquat = texture2D( texture0, gtx ) * 255.;
		gtx += texadvance;
		textra = texture2D( texture0, gtx ) * 255.;
		gtx += texadvance;
		tpos += texture2D( texture0, gtx ) * 255. / 256.;
		tpos = ( tpos - 128. ) * (256./2048.);

		gtx += texadvance;
		tquat += texture2D( texture0, gtx ) * 255. / 256.;
		tquat =  ( tquat - 128. ) * (256./32768.);

		gtx += texadvance;
		textra = texture2D( texture0, gtx ) * 255. / 256.;
		textra =  ( textra - 128. ) * (256./2048.);
		vvExtra = textra;

		vert = (vert + 2.0 * cross( tquat.xyz, cross(tquat.xyz, vert) + tquat.w * vert ) ) * tpos.w + tpos.xyz;
//		vert = vert * tpos.w + tpos.xyz;
	}

	vec4 vvpos = mmatrix * vec4(vert, 1.0);
	vec4 outpos = (pmatrix * (vmatrix * vvpos ) );

	vv0Pos = vvpos;
#if 1
	gl_Position = outpos;
#else
	vec2 rscreenpos = outpos.xy/outpos.w;

	//My awful method for trying to correct for lens warp.
	float compb = dot(rscreenpos,rscreenpos);
	compb = min( compb, 2.0 );
	rscreenpos *= (1.0-  compb*.1);
	gl_Position.xy = rscreenpos * gl_Position.w;
#endif
}


