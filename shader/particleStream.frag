#version 460
in float Transp;
in vec2 TexCoord;

layout(location = 0) out vec4 FragColour;
layout(binding = 6) uniform sampler2D ParticleTex;

void main(){
    FragColour = texture(ParticleTex, TexCoord);
    FragColour.a *= Transp;
}