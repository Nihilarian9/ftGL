#version 330 core
out vec4 FragColour;

in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

void main()
{

	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture1, TexCoord).r);
	FragColour = sampled * vec4(0.1, 0.9, 0.1, 1.0);

	//FragColour = texture(texture1, TexCoord);
	//FragColour = vec4(0.5, 0.6, 0.7, 1.0);

}
