#version 460

layout (location = 0) out vec4 FragColour;

layout(binding = 0) uniform samplerCube SkyboxDay;
layout(binding = 1) uniform samplerCube SkyboxNight;

in vec3 Position;

uniform float Time;
uniform float FullCycleDuration;
uniform float BlendFactor;


void main() {    
    float angle = -(Time / FullCycleDuration) * 1.0 * 3.1415926;
    
    mat3 rotation = mat3(
        cos(angle), 0.0, -sin(angle),
        0.0,        1.0,  0.0,
        sin(angle), 0.0,  cos(angle)
    );

    vec3 rotatedDir = rotation * Position;

    // Sample both skyboxes
    vec3 dayColour = texture(SkyboxDay, rotatedDir).rgb;
    vec3 nightColour = texture(SkyboxNight, rotatedDir).rgb;

    // Blend and apply gamma correction if needed
    vec3 blended = mix(dayColour, nightColour, BlendFactor);
    float gammaCorrection = mix(1.0 / 0.9, 1.0 / 0.4, BlendFactor);
    vec3 gammaCorrected = pow(blended, vec3(gammaCorrection));

    FragColour = vec4(gammaCorrected, 1.0);
}


