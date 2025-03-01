#version 460

in vec2 TexCoord;
in vec3 FragWorldPos;


layout(binding = 5) uniform sampler2D spriteTex;

layout(location = 0) out vec4 FragColour;

void main() {
    
    vec4 texColor = texture(spriteTex, TexCoord);    
    
    if (texColor.a < 0.1) {
        discard;
    }
    FragColour = texColor;
}


