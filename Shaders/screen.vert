#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNor;
layout(location = 2) in vec2 inTexCoord;

out vec2 chTexCoord;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main()
{
    gl_Position = uP * uV * uM * vec4(inPos, 1.0);
    chTexCoord = inTexCoord;
}
