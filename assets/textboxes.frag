#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;
varying vec4 vvExtra;


void main()
{
    gl_FragColor = vec4( vvExtra.zy+vv1Col.xy, 1.0, 1.0);
}
