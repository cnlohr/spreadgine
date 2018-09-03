#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;


void main()
{
    gl_FragColor = vec4( vvExtra.zzy+vv1Col.xyz, 1.0);
}
