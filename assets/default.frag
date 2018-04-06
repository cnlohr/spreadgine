#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vvColor;
varying vec4 vvPos;

void main()
{
    gl_FragColor = vec4( vvColor.xyz, 1.0 );
}
