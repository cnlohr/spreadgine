#version 330


in vec3 pos;
uniform mat4 pmatrix, vmatrix, mmatrix;

void main()
{
    gl_Position = (pmatrix * 
		(vmatrix * 
		(mmatrix * vec4(pos.xyz, 1.0))
	));

//	gl_Position = vec4(pos, 0.0, 1.0);
}
