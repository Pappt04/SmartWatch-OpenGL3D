#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor; //Normale
layout(location = 2) in vec2 inTexCoord; //Texture coordinates

out vec3 chFragPos; //Interpolirana pozicija fragmenta
out vec3 chNor; //Interpolirane normale
out vec2 chTexCoord; //Interpolirane texture koordinate

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main()
{
	chFragPos = vec3(uM * vec4(inPos, 1.0));
	gl_Position = uP * uV * vec4(chFragPos, 1.0);
	chNor = mat3(transpose(inverse(uM))) * inNor; //Inverziju matrica bolje racunati na CPU
	chTexCoord = inTexCoord;
}