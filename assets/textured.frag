#ifdef GL_ES
	precision mediump float;
#endif

uniform vec4      texSize0;
uniform sampler2D texture0;
varying vec4 vv2Tex;
varying vec4 vv1Col;
varying vec4 vv0Pos;

void main()
{
//	gl_FragColor = vec4( vv2Tex.xy, 1., 1. );
//	return;
	vec4 init_tex = texture2D( texture0, vv2Tex.xy );
//	gl_FragColor = vec4( init_tex.rg,0.5, 1. );
//	return;
	if( init_tex.a == 0. )
	{
		gl_FragColor = vec4( init_tex.rgb, 1.0 );
	}
	else
	{
		vec2 ts = texSize0.xy;
		vec2 tloc = init_tex.xy * 2.0 / vec2(ts) + 0.8;
		vec4 sub_tex = texture2D( texture0, tloc );
	    gl_FragColor = vec4( sub_tex.xyz, 1.0 );
	}
}
