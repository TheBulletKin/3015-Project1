#version 460

layout (location = 0) out vec4 FragColour;

layout(binding = 3) uniform samplerCube SkyboxTex;

in vec3 Position;

uniform float Time;
uniform float FullCycleDuration;


void main() {    
    float angle = -(Time / FullCycleDuration) * 1.0 * 3.1415926;
    
    mat3 rotation = mat3(
        cos(angle), 0.0, -sin(angle),
        0.0,        1.0,  0.0,
        sin(angle), 0.0,  cos(angle)
    );

    vec3 rotatedDir = rotation * Position;

    vec3 texColour = texture(SkyboxTex, rotatedDir).rgb;
    texColour = pow(texColour, vec3(1.0 / 0.5));

    FragColour = vec4(texColour, 1.0);
}


