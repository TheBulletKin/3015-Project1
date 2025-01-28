#version 460


layout (location = 0) out vec4 FragColor;

in vec3 LightIntensity;
in vec3 AmbientLight;
in vec3 DiffuseLight;
in vec3 SpecularLight;

void main() {
    vec3 combined = AmbientLight + DiffuseLight + SpecularLight;

    FragColor = vec4(combined, 1.0f);
}
