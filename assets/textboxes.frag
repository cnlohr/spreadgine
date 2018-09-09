#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;
uniform vec4 fontspot;
uniform sampler2D texture0;

void main()
{
	vec2 fvpos = vv1Col.xy * fontspot.zw + fontspot.xy;
	vec4 tv   = texture2D( texture0, fvpos );
    gl_FragColor = vec4( fontspot.zw*1000.0, 0.0, 1.0);
}
