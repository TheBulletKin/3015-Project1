#version 460

layout (location = 0) out vec4 FragColor;



layout(binding = 5) uniform samplerCube SkyboxTex;

uniform vec3 ViewPos;

in vec3 Vec;



void main() {
    vec3 texColour = texture(SkyboxTex, vec3(Vec.x, Vec.y, Vec.z)).rgb;
    texColour = pow(texColour, vec3(1.0/2.2));
    FragColor = vec4(texColour, 1.0);
}


