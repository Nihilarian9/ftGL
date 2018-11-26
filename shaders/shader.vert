#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;

void main()
{
	gl_Position = model * projection * vec4(vertex.xy, 0.0, 1.0);
	TexCoord = vertex.zw;

	//TexCoord.x = vertex.z;
	//TexCoord.y = 1.0 - vertex.w;
}
