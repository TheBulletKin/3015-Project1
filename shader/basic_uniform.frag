#version 460


layout (location = 0) out vec4 FragColor;

in vec3 Colour;

void main() {
    

    FragColor = vec4(Colour, 1.0f);
}
