#version 330 core

in vec2 chTexCoord;
out vec4 outCol;

uniform sampler2D uScreenTexture;
uniform float uBrightness;

void main()
{
    vec4 texColor = texture(uScreenTexture, chTexCoord);
    outCol = texColor * uBrightness;
}
