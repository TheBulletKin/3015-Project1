#version 460

in vec2 TexCoord;
in vec3 FragWorldPos;


layout(binding = 3) uniform sampler2D spriteTex;

layout(location = 0) out vec4 FragColour;

void main() {
    FragColour = texture(spriteTex, TexCoord);
}


