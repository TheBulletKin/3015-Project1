#version 460

layout (location = 0) out vec4 FragColour;

layout(binding = 3) uniform samplerCube SkyboxTex;

in vec3 Position;

void main() {
    vec3 texColour = texture(SkyboxTex, vec3(Position.x, Position.y, Position.z)).rgb;
    texColour = pow(texColour, vec3(1.0/0.9));
    FragColour = vec4(texColour, 1.0);
}


