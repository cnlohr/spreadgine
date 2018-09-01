#version 330

layout(points) in;
layout(triangle_strip, max_vertices = 1024) out;

void main()
{
	for( int i = 0; i < 100; i++ )
	{
		float fv = i * 3.14159 * .02;
	    gl_Position = gl_in[0].gl_Position + vec4(sin(fv)*-5, cos(fv)*-5, 0.0, 0.0);
    	EmitVertex();

    	gl_Position = gl_in[0].gl_Position + vec4(sin(fv)*5., cos(fv)*5., 0.0, 0.0);
    	EmitVertex();

    	gl_Position = gl_in[0].gl_Position + vec4(sin(fv+.02)*5., cos(fv+.02)*5., 0.0, 0.0);
    	EmitVertex();


	    EndPrimitive();
	}

}

/*
in gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
*/
