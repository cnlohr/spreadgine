#ifdef GL_ES
	precision mediump float;
#endif

varying vec4 vv1Col;


void main()
{
    gl_FragColor = vec4(vv1Col.xyz, 1.0);
}
